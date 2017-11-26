#pragma once

#include "hazard_pointers.h"

#include <atomic>
#include <mutex>

namespace Reactive {

using std::atomic;
using std::mutex;

// Unordered set.
// Element type T must support euqality comparison (operator '==').

template <typename T>
class Set
{
private:
    struct Node
    {
        atomic<Node*> next { nullptr };
        atomic<bool> removed { false };
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

        // If value already is already in the set, abort.
        for(Node * n = head.next; n != nullptr; n = n->next)
        {
            if (n->value == value)
                return;
        }

        // Create node
        Node *node = new Node;
        node->value = value;

        // Insert nodes in increasing order of their addresses
        Node * prev = &head;
        while(prev->next && prev->next < node)
            prev = prev->next;

        node->next = prev->next.load();
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
                cur->removed = true;
                Hazard_Pointers::reclaim(cur);
                return true;
            }

            prev = cur;
        }

        return false;
    }

    // Blocking
    // O(N)

    void clear()
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * n = head.next;
        head.next = nullptr;
        while(n)
        {
            Node * next = n->next;
            n->removed = true;
            Hazard_Pointers::reclaim(n);
            n = next;
        }
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
        Iterator(Node * head): head(head)
        {
            hp0.pointer = head;
            hp1.pointer = nullptr;
        }

        // End iterator
        Iterator()
        {}

        Iterator(const Iterator & other)
        {
            head = other.head;
            hp0.pointer = other.hp0.pointer.load();
            hp1.pointer = nullptr;
        }

        Iterator & operator=(const Iterator & other)
        {
            head = other.head;
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
            return hp0.pointer == other.hp0.pointer;
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

            Node * current;
            Node * next;

            current = h0.load();

            do
            {
                next = current->next;
                h1 = next;

                if (current->removed)
                {
                    // Restart from head
                    h0 = current = head;
                    continue;
                }
                else if (current->next == next)
                {
                    // Successfully got next, so make it current
                    h0 = current = next;
                    // Stop if this is a previously unvisited element
                    if (!current || current > last_visited_pos)
                        break;
                }
                // Repeat for the current element
            }
            while(true);

            // Remember position of current element
            last_visited_pos = current;

            return *this;
        }

    private:
        Node * head = nullptr;
        Node * last_visited_pos = nullptr;
        Hazard_Pointer<Node> & hp0 = Hazard_Pointers::acquire<Node>();
        Hazard_Pointer<Node> & hp1 = Hazard_Pointers::acquire<Node>();
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
