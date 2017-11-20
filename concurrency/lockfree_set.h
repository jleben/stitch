#pragma once

#include "hazard_pointers.h"

#include <atomic>
#include <mutex>

namespace Concurrent {
namespace Lockfree {

using std::atomic;
using std::mutex;

// Unordered set

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
    // Blocking
    // O(N)

    void insert(const T & value)
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * n = head.next;
        while(n)
        {
            if (n->value == value)
                return;
            n = n->next;
        }

        n = new Node;
        n->value = value;
        n->next = head.next.load();
        head.next = n;
    }

    // Blocking
    // O(N)

    bool remove(const T & value)
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * a, * b;
        a = &head;
        while((b = a->next))
        {
            if (b->value == value)
            {
                a->next = b->next.load();
                Hazard_Pointers::reclaim(b);
                return true;
            }

            a = b;
        }

        return false;
    }


    // Lockfree
    // O(N)

    bool contains(const T & value)
    {
        auto & h0 = Hazard_Pointers::h<Node>(0);
        auto & h1 = Hazard_Pointers::h<Node>(1);

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

        return found;
    }

    // Lockfree
    // O(N)

    template <typename F>
    void for_each(F f)
    {
        auto & h0 = Hazard_Pointers::h<Node>(0);
        auto & h1 = Hazard_Pointers::h<Node>(1);

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
    }
};

}

}
