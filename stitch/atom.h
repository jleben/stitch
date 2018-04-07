#pragma once

#include <atomic>
#include <cstdint>

#include "hazard_pointers.h"

namespace Stitch {

using std::atomic;

template <typename T> class AtomWriter;
template <typename T> class AtomReader;

template <typename T>
class Atom
{
    friend class AtomWriter<T>;
    friend class AtomReader<T>;

public:
    static bool is_lockfree()
    {
        atomic<Head> head;
        return head.is_lock_free();
    }

    Atom():
        d_current(new Node(1))
    {}

    Atom(T value):
        d_current(new Node(value, 1))
    {}

    ~Atom()
    {
        delete d_current.load();
    }

private:
    struct Node
    {
        Node(int refcount): ref(refcount) {}
        Node(const T & value, int refcount): value(value), ref(refcount) {}

        Node * next = nullptr;
        T value {};
        atomic<int> ref;
    };

    struct Head
    {
        uintptr_t version;
        Node * first = nullptr;
    };

    // If 'node' is current, just return it.
    // Otherwise, unref 'node', get current, ref it and return it.
    Node * get_current(Node * node)
    {
        Node * c = d_current.load();

        if (c == node)
            return node;

        unref(node);

        Detail::Hazard_Pointer<Node> & hp = Detail::Hazard_Pointers::acquire<Node>();
        auto & h = hp.pointer;

        for(;; c = d_current.load())
        {
            // Protect the node being deleted while we inspect it.
            h = c;
            if (d_current != h)
                continue;
            // If reference count is 0, then the current node has just been
            // replaced and either returned into free list or reclaimed.
            // So only increase the ref count and use the node if
            // its reference count is larger than 0.
            int ref = c->ref.load();
            if (ref == 0)
                continue;
            if (c->ref.compare_exchange_weak(ref, ref+1))
                break;
        }

        h = nullptr;
        hp.release();

        return c;
    }

    // Assuming: node has reference count 0
    // Set reference count to 1 and make current.
    // Reduce reference count of old current.
    // Acquire and return new node.
    Node * make_current(Node * n)
    {
        n->ref.store(1);

        Node * old = d_current.exchange(n);

        unref(old);

        return acquire();
    }

    // Reduce reference count and release node if count is 0
    void unref(Node * n)
    {
        n->ref.fetch_sub(1);

        if (n->ref.load() == 0)
        {
            release(n);
        }
    }

    // Put node into free list
    void release(Node * n)
    {
        for(;;)
        {
            auto head = d_free.load();
            n->next = head.first;
            Head new_head { head.version + 1, n };
            if(d_free.compare_exchange_weak(head, new_head))
                break;
        }
    }

    // Get node from free list
    Node * acquire()
    {
        // Only one thread ever acquires, so there's no ABA
        // FIXME: An observer needs to acquire when disconnected.
        for (;;)
        {
            auto head = d_free.load();
            if (!head.first)
                return nullptr;
            Head new_head { head.version + 1, head.first->next };
            if(d_free.compare_exchange_weak(head, new_head))
                return head.first;
        }
    }

    // FIXME: Store initial node into d_current?
    atomic<Node*> d_current { nullptr };
    atomic<Head> d_free;
};

template <typename T>
class AtomWriter
{
    using Node = typename Atom<T>::Node;

public:
    AtomWriter(Atom<T> & atom, T value = T()):
        d_atom(atom),
        d_node(new Node(value, 0))
    {}

    ~AtomWriter()
    {
        // Since we allocated a node in constructor,
        // reclaim one now.
        Detail::Hazard_Pointers::reclaim(d_node);
    }

    T & value() { return d_node->value; }

    void store()
    {
        d_node = d_atom.make_current(d_node);
    }

    void store(const T & value)
    {
        this->value() = value;
        store();
    }

private:
    Atom<T> & d_atom;
    Node * d_node;
};

template <typename T>
class AtomReader
{
    using Node = typename Atom<T>::Node;

public:
    AtomReader(Atom<T> & atom, T value = T()):
        d_atom(atom),
        d_node(new Node(value, 1))
    {}

    ~AtomReader()
    {
        d_atom.unref(d_node);

        // Since we allocated a node in constructor,
        // reclaim one now.
        Node * node = d_atom.acquire();
        Detail::Hazard_Pointers::reclaim(node);
    }

    const T & value() { return d_node->value; }

    const T & load()
    {
        d_node = d_atom.get_current(d_node);
        return d_node->value;
    }

private:
    Atom<T> & d_atom;
    Node * d_node;
};

}
