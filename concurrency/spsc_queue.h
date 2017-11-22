#include "queue.h"
#include "signal.h"

#include <atomic>
#include <vector>

namespace Reactive {

using std::vector;
using std::atomic;

// FIXME: Relax memory ordering constraints

template <typename T>
class SPSC_Queue : public Queue<T>
{
public:
    SPSC_Queue(int capacity): d_data(capacity + 1) {}

    static bool is_lockfree()
    {
        return ATOMIC_INT_LOCK_FREE;
    }

    int capacity() const
    {
        return (int) d_data.size()-1;
    }

    bool full() override
    {
        return !writable_size();
    }

    bool empty() override
    {
        return !readable_size();
    }

    bool push(const T & value) override
    {
        if (!writable_size())
            return false;

        d_data[d_write_pos] = value;
        advance_write(1);
        d_signal.notify();
        return true;
    }

    template <typename F>
    bool push(int count, F producer)
    {
        if (writable_size() < count)
            return false;

        int w = d_write_pos;
        int s = d_data.size();

        if (w + count > s)
        {
            int wrap_count = w + count - s;
            for (; w < s; ++w)
            {
                d_data[w] = producer();
            }
            for (w = 0; w < wrap_count; ++w)
            {
                d_data[w] = producer();
            }
        }
        else
        {
            int end = w + count;
            for (; w < end; ++w)
            {
                d_data[w] = producer();
            }
        }

        d_write_pos = w;
        d_signal.notify();
        return true;
    }

    bool pop(T & value) override
    {
        if (!readable_size())
            return false;

        value = d_data[d_read_pos];
        advance_read(1);
        return true;
    }

    template <typename F>
    bool pop(int count, F consumer)
    {
        if (readable_size() < count)
            return false;

        int r = d_read_pos;
        int s = d_data.size();

        if (r + count > s)
        {
            int wrap_count = r + count - s;
            for (; r < s; ++r)
            {
                consumer(d_data[r]);
            }
            for (r = 0; r < wrap_count; ++r)
            {
                consumer(d_data[r]);
            }
        }
        else
        {
            int end = r + count;
            for (; r < end; ++r)
            {
                consumer(d_data[r]);
            }
        }

        d_read_pos = r;
        return true;
    }

    Event write_event() { return d_signal.event(); }

private:
    int readable_size() const
    {
        int s = d_data.size();

        if (!s)
            return 0;

        int r = d_read_pos;
        int w = d_write_pos;
        int count = (s + w - r) % s;
        return count;
    }

    int writable_size() const
    {
        int s = d_data.size();

        if (!s)
            return 0;

        int r = d_read_pos;
        int w = d_write_pos;
        int count = (s + r - w - 1) % s;
        return count;
    }

    void advance_write(int count)
    {
        int w = d_write_pos;
        d_write_pos = (d_write_pos + count) % d_data.size();
    }


    void advance_read(int count)
    {
        d_read_pos = (d_read_pos + count) % d_data.size();
    }

    atomic<int> d_write_pos { 0 };
    atomic<int> d_read_pos { 0 };
    vector<T> d_data;
    Signal d_signal;
};

}
