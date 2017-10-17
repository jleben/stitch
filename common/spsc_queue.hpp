#include "queue.hpp"

namespace Reactive {

// FIXME: Relax memory ordering constraints

template <typename T>
class SPSC_Queue : public Queue<T>
{
public:
    SPSC_Queue(int size):
        d_data(size) {}

    bool empty() override
    {
        return !readable_size();
    }

    void push(const T & value) override
    {
        if (!writable_size())
            throw std::out_of_range("Full.");

        d_data[d_write_pos] = value;
        advance_write(1);
    }

    T pop() override
    {
        if (!readable_size())
            throw std::out_of_range("Empty.");

        T value = d_data[d_read_pos];
        advance_read(1);
        return value;
    }

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
};

}
