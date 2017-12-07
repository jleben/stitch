#pragma once

#include <cstdint>
#include <vector>
#include <atomic>
#include <cstdio>

namespace Stitch {

using std::vector;
using std::atomic;

template<typename T>
class Lockfree_MPMC_Queue
{
public:
    Lockfree_MPMC_Queue(int capacity):
        d_data(next_power_of_two(capacity)),
        d_pos_mask(d_data.size() - 1)
    {}

    bool push(const T& value)
    {
        int pos;

        while(true)
        {
            uint64_t iter = d_write_pos.load();
            pos = iter & d_pos_mask;
            auto state = d_data[pos].state.load();
            if (state == Full)
            {
                if (iter == d_write_pos.load())
                    return false;
                else
                    continue;
            }
            if (d_write_pos.compare_exchange_weak(iter, iter + 1))
                break;
        }

        d_data[pos].value = value;
        d_data[pos].state = Full;

        return true;
    }

    bool pop(T & value)
    {
        int pos;

        while(true)
        {
            uint64_t iter = d_read_pos.load();
            pos = iter & d_pos_mask;
            auto state = d_data[pos].state.load();
            if (state == Empty)
            {
                if (iter == d_read_pos.load())
                    return false;
                else
                    continue;
            }
            if (d_read_pos.compare_exchange_weak(iter, iter + 1))
                break;
        }

        value = d_data[pos].value;
        d_data[pos].state = Empty;

        return true;
    }

private:
    enum State
    {
        Empty,
        Full,
    };

    struct Element
    {
        atomic<State> state { Empty };
        T value;
    };

    uint64_t next_power_of_two(uint64_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;
        return v;
    }

    vector<Element> d_data;
    atomic<uint64_t> d_write_pos { 0 };
    atomic<uint64_t> d_read_pos { 0 };
    uint64_t d_pos_mask;
};

}
