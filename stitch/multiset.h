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
class Multiset
{
    using Hazard_Pointers = Detail::Hazard_Pointers;
    template <typename P> using Hazard_Pointer = Detail::Hazard_Pointer<P>;

private:

    struct Node;

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

    Multiset() {}

    /*!
     * \brief Destructor.
     *
     * - Progress: Blocking
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

    ~Multiset()
    {
        clear();
    }

    // Delete copy constructor and assignment operator

    Multiset(const Multiset &) = delete;
    Multiset & operator=(const Multiset &) = delete;

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

    enum Mark
    {
        Node_To_Remove = 1,
        Node_Removed = 2
    };

    // We assume any Node * is aligned at least to 4 bytes.
    // So we have the last 2 bits of space for the mark.
    static int mark(Node * p)
    {
        return uintptr_t(p) bitand 3;
    }

    static bool is_marked(Node * p)
    {
        return mark(p) != 0;
    }

    static Node * marked(Node * p, Mark mark)
    {
        return (Node*)(~(~uintptr_t(p) | 3) | mark);
    }

    static Node * unmarked(Node * p)
    {
        return (Node*)(~(~uintptr_t(p) | 3));
    }

private:
    struct Internal_Iterator
    {
        Internal_Iterator():
            h0(Hazard_Pointers::acquire<Node>()),
            h1(Hazard_Pointers::acquire<Node>())
        {}

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
            auto & hp0 = h0.pointer;
            auto & hp1 = h1.pointer;
start:
            for(;;)
            {
                printf("Start\n");

                prev = head;
                cur = prev->next.load();

                while(cur != nullptr)
                {
                    printf("Cur %p\n", cur);

                    hp1.store(cur);

                    // cur is safe if there is a link to it (prev->next)
                    // which is reachable (prev->next is not marked, so node has not been removed).
                    if (prev->next.load() != cur)
                        goto start;

                    printf("Safe\n");

                    next = cur->next.load();

                    if (is_marked(next))
                    {
                        printf("Marked\n");

                        if (!prev->next.compare_exchange_strong(cur, unmarked(next)))
                        {
                            printf("Failed to remove\n");

                            // Either someone else has already removed cur,
                            // or prev has been removed and prev->next is marked.

                            // Could be optimized: if prev->next is not marked,
                            // just retry with cur = prev->next
                            goto start;
                        }

                        printf("Removed\n");

                        cur->next.store(marked(next, Node_Removed));

                        Hazard_Pointers::reclaim(cur);

                        cur = next;
                        continue;
                    }

                    if (func(cur))
                    {
                        printf("Found\n");
                        return true;
                    }

                    printf("This is not the node you are looking for.\n");

                    hp0.store(cur);
                    prev = cur;
                    cur = next;
                }

                return false;
            }
        }

    private:
        Hazard_Pointer<Node> & h0;
        Hazard_Pointer<Node> & h1;

    public:
        Link * prev;
        Node * cur;
        Node * next;
    };

public:

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

        Internal_Iterator iter;

        for(;;)
        {
            // Find first node with address larger than new node's.
            iter.find(&head, [&](Node * cur)
            {
                return cur > node;
            });

            // Try insert node before found node
            node->next.store(iter.cur);

            if(iter.prev->next.compare_exchange_strong(iter.cur, node))
                return;
        }
    }


    /*!
     * \brief Removes the given value if it is in the set.
     *
     * Returns whether the value was in the set.
     *
     * - Progress: Lockfree, if allocator is lockfree.
     * - Time complexity: Asymptotic O(N). Worst-case O(N + H).
     */

    bool remove(const T & value)
    {
        Internal_Iterator iter;

        for(;;)
        {
            bool was_found = iter.find(&head, [&](Node * node)
            {
                return node->value == value;
            });

            if (!was_found)
                return false;

            // Try mark this for removal
            if (iter.cur->next.compare_exchange_strong(iter.next, marked(iter.next, Node_To_Remove)))
                break;

            // Someone else has marked this node for removal.
            // We have to find ourselves another one, so restart.
        }

        if (iter.prev->next.compare_exchange_strong(iter.cur, iter.next))
        {
            // The node is not reachable, so it can be reclaimed.
            // (If it can't, never mind, someone else will reclaim it).
            // FIXME: Release hazard pointers owned by iter, to reclaim as much as possible.
            // FIXME: Make Hazard_Pointers::reclaim lockfree!
            iter.cur->next.store(marked(iter.next, Node_Removed));
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
        // FIXME
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
        Iterator(Link * head): head(head)
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
            hp1.pointer = other.hp1.pointer.load();
        }

        /*!
         * - Progress: Wait-free.
         * - Time complexity: O(1).
         */
        Iterator & operator=(const Iterator & other)
        {
            head = other.head;
            hp0.pointer = other.hp0.pointer.load();
            hp1.pointer = other.hp1.pointer.load();
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
            return hp1.pointer == other.hp1.pointer;
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
            return hp1.pointer.load()->value;
        }

        /*!
         * - Progress: Lock-free.
         * - Time complexity: O(N + K).
         */
        Iterator & operator++()
        {
            auto &h0 = hp0.pointer;
            auto &h1 = hp1.pointer;

            Link * current;
            Node * next;

            current = h0.load();

            do
            {
                next = current->next;
                h1 = next;

                // FIXME: With multiple concurrent executions of 'remove',
                // there is no way to ensure 'current' node has not been removed
                // before hazard pointer was set to 'next',
                // other than by checking a property that 'remove' sets *before*
                // the removal (e.g. marking 'next' as 'Node_To_Remove'.

                // We need another mechanism, like reference counters on nodes,
                // but then the problem is avoiding node deallocation during
                // iteration.

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
        Link * head = nullptr;
        void * last_visited_pos = nullptr;
        // Current node as link
        Hazard_Pointer<Link> & hp0 = Hazard_Pointers::acquire<Link>();
        // Current node as Node (null when hp0 is head)
        Hazard_Pointer<Node> & hp1 = Hazard_Pointers::acquire<Node>();
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
