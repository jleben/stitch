#include "queue.h"
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
class MPMC_Queue : public Queue<T>
{
public:
    static bool is_lockfree()
    {
        return ATOMIC_INT_LOCK_FREE && ATOMIC_BOOL_LOCK_FREE;
    }

    MPMC_Queue(int size):
        d_data(next_power_of_two(size)),
        d_journal(d_data.size()),
        d_wrap_mask(d_data.size() - 1),
        d_readable(0),
        d_writable(d_data.size() - 1)
    {
        for (auto & val : d_journal)
            val = false;

        d_worker = std::thread(&MPMC_Queue::work, this);
    }

    ~MPMC_Queue()
    {
        d_quit = true;
        d_io_event.notify();
        d_worker.join();
    }

    bool full() override
    {
        return d_writable < 1;
    }

    bool empty() override
    {
        return d_readable < 1;
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

        d_data[pos] = value;
        d_journal[pos] = true;

        d_io_event.notify();

        return true;
    }

    bool pop(T & value) override
    {
        int readable = d_readable.fetch_sub(1);
        bool ok = readable > 0;
        if (!ok)
        {
            d_readable.fetch_add(1);
            return false;
        }

        int pos = d_tail.fetch_add(1) & d_wrap_mask;
        d_tail.fetch_and(d_wrap_mask);

        value = d_data[pos];
        d_journal[pos] = false;

        d_io_event.notify();

        return true;
    }

    Signal & event() { return d_public_io_event; }

private:
    int next_power_of_two(int value)
    {
        return std::pow(2, std::ceil(std::log2(value)));
    }

    void work()
    {
        while(!d_quit)
        {
            bool changed = false;

            while(d_journal[d_head_probe])
            {
                d_head_probe = (d_head_probe + 1) & d_wrap_mask;
                d_readable.fetch_add(1);
                changed = true;
            }
            while(d_tail_probe != d_head_probe && !d_journal[d_tail_probe])
            {
                d_tail_probe = (d_tail_probe + 1) & d_wrap_mask;
                d_writable.fetch_add(1);
                changed = true;
            }

            if (changed)
                d_public_io_event.notify();

            d_io_event.wait();
        }
    }

    vector<T> d_data;
    vector<atomic<bool>> d_journal;
    int d_wrap_mask = 0;

    atomic<int> d_head { 0 };
    atomic<int> d_tail { 0 };

    int d_head_probe { 0 };
    int d_tail_probe { 0 };

    atomic<int> d_readable { 0 };
    atomic<int> d_writable { 0 };

    std::thread d_worker;
    atomic<bool> d_quit { false };
    Signal d_io_event;
    Signal d_public_io_event;
};

}
