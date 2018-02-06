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

    template <typename I>
    bool push(int count, const I & input_start)
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

    template <typename O>
    bool pop(int count, const O & output_start)
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

