#include <cmath>
#include <atomic>
#include <vector>
#include <thread>
#include <stdexcept>

namespace Stitch {

using std::vector;
using std::atomic;

// FIXME: Use atomic_flag instead of atomic<bool>

template <typename T>
class Waitfree_MPSC_Queue
{
public:
    static bool is_lockfree()
    {
        return (ATOMIC_INT_LOCK_FREE == 2) && (ATOMIC_BOOL_LOCK_FREE == 2);
    }

    Waitfree_MPSC_Queue(int size):
        d_data(next_power_of_two(size)),
        d_journal(d_data.size()),
        d_wrap_mask(d_data.size() - 1),
        d_writable(d_data.size())
    {
        //printf("Size = %d\n", (int) d_data.size());

        for (auto & val : d_journal)
            val = false;
    }

    ~Waitfree_MPSC_Queue()
    {}

    int capacity() const
    {
        return d_data.size();
    }

    bool full()
    {
        return d_writable < 1;
    }

    bool empty()
    {
        return d_journal[d_tail] == false;
    }

    /*!
    \brief Adds an item to the queue.

    The item \p value is added to the input end of the queue.

    This can fail if the queue is full, in which case nothing is done.

    \return True on success, false on failure.

    - Progess: Wait-free
    - Time complexity: O(1)
    */
    bool push(const T & value)
    {
        int pos;
        if (!reserve_write(1, pos))
            return false;

        //printf("Writing at %d\n", pos);

        d_data[pos] = value;
        d_journal[pos] = true;

        return true;
    }

    /*!
    \brief Adds items in bulk to the queue.

    'count' consecutive item starting at the 'input_start' iterator are added to the input end of the queue.

    This can fail if the queue does not have space for 'count' items, in which case nothing is done.

    The 'input_start' type 'I' must satisfy the Input Iterator requirements, with `value_type` equal to T.
    For example `T*`.
    See: https://en.cppreference.com/w/cpp/named_req/InputIterator

    \return True on success, false on failure.

    For example:

        Waitfree_MPSC_Queue<int> q(10);
        int data[5];
        q.push(5, data);

    - Progess: Wait-free
    - Time complexity: O(count)
    */
    template <typename I>
    bool push(int count, I input_start)
    {
        int pos;
        if (!reserve_write(count, pos))
            return false;

        I input = input_start;

        for (int i = 0; i < count; ++i, ++input)
        {
            d_data[pos] = *input;
            d_journal[pos] = true;
            pos = (pos + 1) & d_wrap_mask;
        }

        return true;
    }

    /*!
    \brief
    Removes an item from the queue.

    An item is removed from the output end of the queue and stored in \p value.

    This can fail if the queue is empty, in which case nothing is done.

    \return True on success, false on failure.

    - Progess: Wait-free
    - Time complexity: O(1)
    */

    bool pop(T & value)
    {
        if (!d_journal[d_tail])
            return false;

        int pos = d_tail;
        d_tail = (d_tail + 1) & d_wrap_mask;

        //printf("Reading at %d\n", pos);

        value = d_data[pos];
        d_journal[pos] = false;

        d_writable.fetch_add(1);

        return true;
    }

    /*!
    \brief Removes items in bulk from the queue.

    'count' items are removed from the output end of the queue, and stored into consecutive locations starting at the 'output_start' iterator.

    This can fail if there is less than 'count' items in the queue, in which case nothing is done.

    The 'output_start' type 'O' must satisfy the Output Iterator requirements, with `value_type` equal to T.
    For example `T*`.
    See: https://en.cppreference.com/w/cpp/named_req/OutputIterator

    \return True on success, false on failure.

    For example:

        Waitfree_MPSC_Queue<int> q(10);
        int data[5];
        q.pop(5, data);

    - Progess: Wait-free
    - Time complexity: O(count)
    */

    template <typename O>
    bool pop(int count, O output_start)
    {
        if (count > d_data.size())
            return false;

        int pos = d_tail;

        for (int i = 0; i < count; ++i)
        {
            int j = (pos + i) & d_wrap_mask;
            if (!d_journal[j])
                return false;
        }

        O output = output_start;

        for (int i = 0; i < count; ++i, ++output)
        {
            *output = d_data[pos];
            d_journal[pos] = false;
            pos = (pos + 1) & d_wrap_mask;
        }

        d_tail = pos;
        d_writable.fetch_add(count);

        return true;
    }

private:
    bool reserve_write(int count, int & pos)
    {
        int old_writable = d_writable.fetch_sub(count);
        bool ok = old_writable - count >= 0;
        if (!ok)
        {
            d_writable.fetch_add(count);
            return false;
        }

        // We must wrap "pos", because another thread might have
        // incremented d_head just before us.
        pos = d_head.fetch_add(count) & d_wrap_mask;

        d_head.fetch_and(d_wrap_mask);

        return true;
    }

    int next_power_of_two(int value)
    {
        return std::pow(2, std::ceil(std::log2(value)));
    }

    vector<T> d_data;
    vector<atomic<bool>> d_journal;
    int d_wrap_mask = 0;

    atomic<int> d_head { 0 };
    atomic<int> d_writable { 0 };

    int d_tail { 0 };
};

}

