#include "connections.h"
#include "mpsc_queue.h"

namespace Stitch {

template <typename T>
class Stream_Producer : public Client<MPSC_Queue<T>>
{
public:
    using Queue = MPSC_Queue<T>;

    // Lock-free

    void push(const T & val)
    {
        for (Queue & q : *this) { q.push(val); }
    }

    // Lock-free

    template <typename I>
    void push(int count, const I & input)
    {
        for (Queue & q : *this) { q.push(count, input); }
    }
};

template <typename T>
class Stream_Consumer : public Server<MPSC_Queue<T>>
{
public:
    using Queue = MPSC_Queue<T>;

    Stream_Consumer(int capacity):
        Server<MPSC_Queue<T>>(capacity)
    {}

    // Wait-free

    bool empty()
    {
        return this->data().empty();
    }

    // Wait-free

    bool pop(T & v)
    {
        return this->data().pop(v);
    }

    // Wait-free

    template <typename O>
    bool pop(int count, const O & output)
    {
        return this->data().pop(count, output);
    }

    Event receive_event()
    {
        return this->data().event();
    }
};

template <typename T>
void connect(Stream_Producer<T> &, Stream_Producer<T> &) = delete;

}
