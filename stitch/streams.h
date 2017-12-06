#include "connections.h"
#include "queue_mpsc_waitfree.h"
#include "signal.h"

namespace Stitch {

template <typename T, typename Queue = Waitfree_MPSC_Queue<T>>
struct Stream_Buffer
{
    Stream_Buffer(int capacity): queue(capacity) {}

    Queue queue;
    Signal signal;
};

template <typename T, typename Q = Waitfree_MPSC_Queue<T>>
class Stream_Producer : public Client<Stream_Buffer<T,Q>>
{
public:
    using Buffer = Stitch::Stream_Buffer<T,Q>;

    // Lock-free

    void push(const T & val)
    {
        for (Buffer & buf : *this) { buf.queue.push(val); buf.signal.notify(); }
    }

    // Lock-free

    template <typename I>
    void push(int count, const I & input)
    {
        for (Buffer & buf : *this) { buf.queue.push(count, input); buf.signal.notify(); }
    }
};

template <typename T, typename Q = Waitfree_MPSC_Queue<T>>
class Stream_Consumer : public Server<Stream_Buffer<T,Q>>
{
public:
    using Buffer = Stitch::Stream_Buffer<T,Q>;

    Stream_Consumer(int capacity):
        Server<Buffer>(std::make_shared<Buffer>(capacity))
    {}

    // Wait-free

    bool empty()
    {
        return this->data().queue.empty();
    }

    // Wait-free

    bool pop(T & v)
    {
        return this->data().queue.pop(v);
    }

    // Wait-free

    template <typename O>
    bool pop(int count, const O & output)
    {
        return this->data().queue.pop(count, output);
    }

    Event receive_event()
    {
        return this->data().signal.event();
    }
};

template <typename T, typename Q>
void connect(Stream_Producer<T,Q> &, Stream_Producer<T,Q> &) = delete;

}
