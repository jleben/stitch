#pragma once

#include "hazard_pointers.h"

#include <atomic>
#include <mutex>

namespace Reactive {

using std::atomic;
using std::mutex;

// Ordered set.
// Element type T must support < operation.

template <typename T>
class Set
{
private:
    struct Node
    {
        atomic<Node*> next { nullptr };
        T value;
    };

    Node head;

    mutex d_mux;

public:

    // Wait-free
    // O(1)

    bool empty() const
    {
        return head.next == nullptr;
    }

    // Blocking
    // O(N)

    void insert(const T & value)
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * prev, * next;
        prev = &head;
        while((next = prev->next))
        {
            if (value == next->value)
                return;
            else if (value < next->value)
                break;
            prev = next;
        }

        Node *node = new Node;
        node->value = value;
        node->next = next;

        prev->next = node;
    }

    // Blocking
    // O(N)

    bool remove(const T & value)
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * prev, * cur;
        prev = &head;
        while((cur = prev->next))
        {
            if (cur->value == value)
            {
                prev->next = cur->next.load();
                Hazard_Pointers::reclaim(cur);
                return true;
            }

            prev = cur;
        }

        return false;
    }


    // Lockfree
    // O(N)

    bool contains(const T & value)
    {
        auto & hp0 = Hazard_Pointers::acquire<Node>();
        auto & hp1 = Hazard_Pointers::acquire<Node>();

        auto & h0 = hp0.pointer;
        auto & h1 = hp1.pointer;

        h0 = &head;

        bool found = false;

        while(!found)
        {
            Node * last = h0.load();
            Node * current = last->next;
            if (!current)
                break;

            // Make current safe, and make sure it's reachable
            h1 = current;
            if (last->next != current)
                continue;

            found = current->value == value;

            // Forget last and make current the new last
            h0 = h1.load();
        }

        h0 = h1 = nullptr;

        hp0.release();
        hp1.release();

        return found;
    }

    struct Iterator
    {

        Hazard_Pointer<Node> & hp0 = Hazard_Pointers::acquire<Node>();
        Hazard_Pointer<Node> & hp1 = Hazard_Pointers::acquire<Node>();

        Iterator(Node * node)
        {
            hp0.pointer = node;
            hp1.pointer = nullptr;
        }

        Iterator(const Iterator & other)
        {
            hp0.pointer = other.hp0.pointer.load();
            hp1.pointer = nullptr;
        }

        Iterator & operator=(const Iterator & other)
        {
            hp0.pointer = other.hp0.pointer.load();
            hp1.pointer = nullptr;
            return *this;
        }

        ~Iterator()
        {
            hp0.pointer = hp1.pointer = nullptr;
            hp0.release();
            hp1.release();
        }

        bool operator==(const Iterator & other) const
        {
            return hp0.pointer == hp1.pointer;
        }

        bool operator!=(const Iterator & other) const
        {
            return !(*this == other);
        }

        T & operator*()
        {
            return hp0.pointer.load()->value;
        }

        Iterator & operator++()
        {
            auto &h0 = hp0.pointer;
            auto &h1 = hp1.pointer;

            Node * last, * current;

            // Get current, make it safe, make sure it's still reachable from last.
            do
            {
                last = h0.load();
                current = last->next;
                h1 = current;
            }
            while(last->next != current);

            // Forget last and store current in its place.
            h0 = current;
            h1 = nullptr;

            return *this;
        }
    };

    Iterator begin()
    {
        Iterator it(&head);
        return ++it;
    }

    Iterator end()
    {
        return Iterator(nullptr);
    }

    // Lockfree
    // O(N)

    template <typename F>
    void for_each(F f)
    {
        auto & hp0 = Hazard_Pointers::acquire<Node>();
        auto & hp1 = Hazard_Pointers::acquire<Node>();

        auto &h0 = hp0.pointer;
        auto &h1 = hp1.pointer;

        h0 = &head;

        while(true)
        {
            Node * last = h0.load();
            Node * current = last->next;

            if (!current)
                break;

            // Make current safe, and make sure it's reachable
            h1 = current;
            if (last->next != current)
                continue;

            f(current->value);

            // Forget last and make current the new last
            h0 = h1.load();
        }

        h0 = h1 = nullptr;

        hp0.release();
        hp1.release();
    }
};

}
