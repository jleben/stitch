#include "queue.hpp"
#include "signal.h"

#include <cmath>
#include <atomic>
#include <vector>
#include <thread>
#include <stdexcept>

namespace Reactive {

using std::vector;
using std::atomic;

// FIXME: Use atomic_flag instead of atomic<bool>

template <typename T>
class MPSC_Queue : public Queue<T>
{
public:
    static bool is_lockfree()
    {
        return ATOMIC_INT_LOCK_FREE && ATOMIC_BOOL_LOCK_FREE;
    }

    MPSC_Queue(int size):
        d_data(next_power_of_two(size)),
        d_journal(d_data.size()),
        d_wrap_mask(d_data.size() - 1),
        d_writable(d_data.size())
    {
        //printf("Size = %d\n", (int) d_data.size());

        for (auto & val : d_journal)
            val = false;
    }

    ~MPSC_Queue()
    {}

    bool full() override
    {
        return d_writable < 1;
    }

    bool empty() override
    {
        return d_journal[d_tail] == false;
    }

    bool push(const T & value) override
    {
        int writable = d_writable.fetch_sub(1);
        bool ok = writable > 0;
        if (!ok)
        {
            d_writable.fetch_add(1);
            return false;
        }

        int pos = d_head.fetch_add(1) & d_wrap_mask;
        d_head.fetch_and(d_wrap_mask);

        //printf("Writing at %d\n", pos);

        d_data[pos] = value;
        d_journal[pos] = true;

        // This may be useless, because an earlier push may not have completed.
        d_public_io_event.notify();

        return true;
    }

    bool pop(T & value) override
    {
        if (!d_journal[d_tail])
            return false;

        int pos = d_tail;
        d_tail = (d_tail + 1) & d_wrap_mask;

        //printf("Reading at %d\n", pos);

        value = d_data[pos];
        d_journal[pos] = false;

        d_writable.fetch_add(1);
        d_public_io_event.notify();

        return true;
    }

    Event event() { return d_public_io_event.event(); }

private:
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

    Signal d_public_io_event;
};

}

