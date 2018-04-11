#pragma once

#include "hazard_pointers.h"

#include <atomic>
#include <mutex>

namespace Stitch {

using std::atomic;
using std::mutex;

// Unordered set.
// Element type T must support equality comparison (operator '==').

/*! \brief Unordered multiset.
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

// Uses node link marking by T. L. Harris.
// Before a node is removed, it's next pointer is marked by incrementing
// its numeric value by 1.
// Insertion and removal use CAS on the previous node's next pointer.
// So by marking that pointer, we ensure no node can be removed or inserted
// right after a node that's about to be removed.
// Marking a node before it's removed constitutes a lock: insertion and removal
// can not proceed until the node is actually removed.
// For this reason, iteration preceding an insertion or removal assists
// with removing all marked nodes along the way.

template <typename T>
class Set
{
    using Hazard_Pointers = Detail::Hazard_Pointers;
    using Hazard_Pointer = Detail::Hazard_Pointer;

private:
    struct Link
    {
        atomic<Node*> next { nullptr };
        atomic<bool> removed { false };
    };

    struct Node : Link
    {
        T value;
    };

    Link head;

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
     * - Progress: Lockfree, if allocator is lockfree.
     * - Time complexity: O(N)
     */

    void insert(const T & value)
    {
        // Create node
        Node *node = new Node;
        node->value = value;

        // Insert nodes in increasing order of their addresses

        auto & h0 = Hazard_Pointers::acquire();
        auto & h1 = Hazard_Pointers::acquire();

        auto & hp0 = h0.pointer;
        auto & hp1 = h1.pointer;

start:
        for(;;)
        {
            Link * prev = &head;
            Node * cur = prev->next.load();

            while(cur != nullptr && cur < node)
            {
                hp1.store(cur);

                // Restart if previous was removed (prev->next is marked)
                // or current was removed
                if (prev->next.load() != cur)
                    goto start;

                Node * next = cur->next.load();

                if (next & 1)
                    // Current node was removed
                    goto start;

                hp0.store(cur);
                prev = cur;
                cur = next;
            }

            node->next.store(cur);

            if(!prev->next.compare_exchange_strong(cur, node))
                continue;
        }

        hp0.store(nullptr);
        hp1.store(nullptr);

        h0.release();
        h1.release();

        return;
    }

    struct Internal_Iterator
    {
        Internal_Iterator()
        {
            h0 = Hazard_Pointers::acquire();
            h1 = Hazard_Pointers::acquire();
        }

        ~Internal_Iterator()
        {
            h0.release();
            h1.release();
        }

        // For each node, return true if func(node) returns true.
        // Return false if func(node) never returns true;
        // Along the way, complete removal of nodes that are marked for removal.
        // When finished, prev, cur and next are all unmarked;
        // prev and cur are safe to dereference (guarded by hazard pointers),
        // next is not necessarily safe.
        template <typename F>
        bool find(Link * head, F func)
        {
start:
            for(;;)
            {
                Link * prev = head;
                Node * cur = prev->next.load();
                Node * next;

                while(cur != nullptr)
                {
                    hp1.store(cur);

                    // cur is safe if there is a link to it (prev->next)
                    // which is reachable (prev->next is not marked, so node has not been removed).
                    if (prev->next.load() != cur)
                        goto start;

                    next = cur->next.load();

                    if (next bitand 1)
                    {
                        if (!prev->next.compare_exchange_strong(cur, next xor 1))
                        {
                            // Either someone else has already removed cur,
                            // or prev has been removed and prev->next is marked.
                            goto start;
                        }

                        Hazard_Pointers::reclaim(cur);

                        cur = next;
                        continue;
                    }

                    if (func(Node * cur))
                        return true;

                    hp0.store(cur);
                    prev = cur;
                    cur = next;
                }
            }

            return false;
        }

    private:
        Hazard_Pointer<Node> & h0;
        Hazard_Pointer<Node> & h1;

    public:
        Link * prev;
        Node * cur;
        Node * next;
    };

    /*!
     * \brief Removes the given value if it is in the set.
     *
     * Returns whether the value was in the set.
     *
     * - Progress: Lockfree, if allocator is lockfree.
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

    // FIXME: Make Hazard_Pointers::reclaim lockfree!
    bool remove(const T & value)
    {
        Internal_Iterator iter(&head);

        for(;;)
        {
            bool was_found = iter.find(&head, [&](Node * node)
            {
                node->value == value;
            });

            if (!was_found)
                return false;

            // Try mark this for removal
            if (iter.cur->next.compare_exchange_strong(iter.next, iter.next | 1))
                break;

            // Someone else has marked this node for removal.
            // We have to find ourselves another one, so restart.
        }

        if (iter.prev->next.compare_exchange_strong(iter.cur, iter.next))
        {
            // The node is not reachable, so it can be reclaimed.
            // (If it can't, never mind, someone else will reclaim it).
            Hazard_Pointers::reclaim(iter.cur);
        }

        return true;
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
