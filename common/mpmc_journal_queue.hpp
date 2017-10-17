#include "queue.hpp"

namespace Reactive {

template <typename T>
class MPMC_Journal_Queue : public Queue<T>
{
public:
    void push(const T & value) override
    {
        if (!can_push())
            throw std::out_of_bounds("Full.");

        int pos = d_head.fetch_add(1) % d_data.size();
        d_data[pos] = value;
        d_max_tail.fetch_add(1);
        // FIXME: wrap head and max_tail;
    }

    T pop() override
    {
        if (!can_pop())
            throw std::out_of_bounds("Empty.");

        int pos = d_tail.fetch_add(1) % d_data.size();
        T value = d_data[pos];
        d_max_head.fetch_add(1);
        // FIXME: wrap tail and max_head;
        return value;
    }

private:
    bool can_push()
    {
        int pos = d_head_probe.fetch_add(1);
        // FIXME: check pos;
        bool ok = false;
        d_head_probe.fetch_sub(1);
        return ok;
    }

    bool can_pop()
    {

    }

    atomic<int> d_head { 0 };
    atomic<int> d_head_probe { 0 };
    atomic<int> d_tail { 0 };
    atomic<int> d_tail_probe { 0 };
    atomic<int> d_max_head { 0 };
    atomic<int> d_max_tail { 0 };
    vector<T> d_data;
    vector<bool> d_journal;
};

}
