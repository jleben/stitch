#pragma once

#include "hazard_pointers.h"

#include <atomic>
#include <mutex>

namespace Stitch {

using std::atomic;
using std::mutex;

// Unordered set.
// Element type T must support equality comparison (operator '==').

/*! \brief Unordered set.
 *
 * Element type T must support equality comparison (operator '==').
 *
 * Progress guarantees in method descriptions use the following parameters:
 * - N = Number of elements currently in the set.
 * - K = Number of hazard pointers in use.
 * - H = Maximum allowable number of hazard pointers.
 */

// Main goal: lock-free iteration using an iterator.

// When iterating, the current element could be in in the process of being removed.
// In that case, some implementations assist the removal and attempt to complete it.
// Removal might trigger dynamic memory deallocation though.
// We would like iteration to be lock-free even when memory deallocation is not.
// So we don't assist the removal.

// If the current element was removed, we also can not continue.
// That's because this element's link doesn't make the next node reachable
// from root. Consequently, the next node (or the next of next...)
// might also have been removed and even reclaimed, thus making the link invalid
// before we can even set a hazard pointer to it.
// Instead, we need to restart iteration from head.
// Note: M.M.Michael does the same in his 2004 paper on hazard pointers.

// So we need a way to restart the iteration from head, but skip nodes
// already visited. For a simple approximation to finding whether a node was
// already visited, we order nodes by their address, and assume we visited
// all nodes with address lower than the current node's.
// This is an over-approximation: we may skip some new nodes, but we will
// never re-visit a node.

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

    /*!
     * \brief Default constructor.
     *
     * - Progress: Wait-free
     * - Time complexity: O(1)
     */

    Set() {}

    /*!
     * \brief Destructor.
     *
     * - Progress: Blocking
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

    ~Set()
    {
        clear();
    }

    // Delete copy constructor and assignment operator

    Set(const Set &) = delete;
    Set & operator=(const Set &) = delete;

    /*!
     * \brief Returns whether the set contains no elements.
     *
     * - Progress: Wait-free
     * - Time complexity: O(1)
     */

    bool empty() const
    {
        return head.next == nullptr;
    }

    /*!
     * \brief Inserts the given value if it is not already in the set.
     *
     * - Progress: Blocking
     * - Time complexity: O(N)
     */

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

    /*!
     * \brief Removes the given value if it is in the set.
     *
     * Returns whether the value was in the set.
     *
     * - Progress: Blocking
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

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
                Detail::Hazard_Pointers::reclaim(cur);
                return true;
            }

            prev = cur;
        }

        return false;
    }

    /*!
     * \brief Removes all elements.
     *
     * - Progress: Blocking
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

    void clear()
    {
        std::lock_guard<mutex> lock(d_mux);

        Node * n = head.next;
        head.next = nullptr;
        while(n)
        {
            Node * next = n->next;
            n->removed = true;
            Detail::Hazard_Pointers::reclaim(n);
            n = next;
        }
    }

    /*!
     * \brief Returns whether the given value is in the set.
     *
     * - Progress: Lock-free.
     * - Time complexity: O(N)
     */

    bool contains(const T & value)
    {
        for (const T & v : *this)
        {
            if (v == value)
                return true;
        }

        return false;
    }

    /*!
     * Progress guarantees in method descriptions use the following parameters:
     * - N = Number of elements currently in the set.
     * - K = Number of hazard pointers in use.
     * - H = Maximum allowable number of hazard pointers.
     */
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

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        Iterator & operator=(const Iterator & other)
        {
            head = other.head;
            hp0.pointer = other.hp0.pointer.load();
            hp1.pointer = nullptr;
            return *this;
        }

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        ~Iterator()
        {
            hp0.pointer = hp1.pointer = nullptr;
            hp0.release();
            hp1.release();
        }

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        bool operator==(const Iterator & other) const
        {
            return hp0.pointer == other.hp0.pointer;
        }

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        bool operator!=(const Iterator & other) const
        {
            return !(*this == other);
        }

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        T & operator*()
        {
            return hp0.pointer.load()->value;
        }

        /*!
         * - Progress: Lock-free.
         * - Time complexity: O(N + K).
         */
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
        Detail::Hazard_Pointer<Node> & hp0 = Detail::Hazard_Pointers::acquire<Node>();
        Detail::Hazard_Pointer<Node> & hp1 = Detail::Hazard_Pointers::acquire<Node>();
    };

    /*!
     * - Progress: Wait-free.
     * - Time complexity: O(1).
     */
    Iterator begin()
    {
        Iterator it(&head);
        return ++it;
    }

    /*!
     * - Progress: Wait-free.
     * - Time complexity: O(1).
     */
    Iterator end()
    {
        return Iterator(nullptr);
    }
};

}
