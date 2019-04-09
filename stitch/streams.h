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

    /*! \brief Adds an item to the queues of all connected consumers.

    Calls `push(val)` on each consumer's queue. See: \ref Waitfree_MPSC_Queue::push(const T &).

    - Progress: Lock-free
    - Time complexity: O(C) where C is the number of connected consumers.
    */

    void push(const T & val)
    {
        for (Buffer & buf : *this) { buf.queue.push(val); buf.signal.notify(); }
    }

    /*!
    \brief Adds items in bulk to the queues of all connected consumers.

    Calls `push(count, input)` on each consumer's queue. See: \ref Waitfree_MPSC_Queue::push(int, I).

    For example:

        int data[5];
        Stream_Producer<int> producer;
        producer.push(5, data);

    - Progress: Lock-free
    - Time complexity: O(count * C) where C is the number of connected consumers.
    */
    template <typename I>
    void push(int count, I input)
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

    /*!
        \brief Whether the consumer's queue is empty.

        - Progress: Wait-free
        - Time complexity: O(1)
     */
    bool empty()
    {
        return this->data().queue.empty();
    }

    // Wait-free

    /*!
    \brief Removes an item from the consumer's queue.

    Calls `pop(v)` on the queue and forwards the return value.
    See: \ref Waitfree_MPSC_Queue::pop(T&).

    - Progress: Wait-free
    - Time complexity: O(1)
    */
    bool pop(T & v)
    {
        return this->data().queue.pop(v);
    }

    /*!
    \brief Removes items in bulk from the consumer's queue.

    Calls `pop(count, output)` on the queue and forwards the return value.
    See: \ref Waitfree_MPSC_Queue::pop(int, O).

    For example:

        int data[5];
        Stream_Consumer<int> consumer;
        bool ok = consumer.pop(5, data);

    - Progress: Wait-free
    - Time complexity: O(count)
    */

    template <typename O>
    bool pop(int count, O output)
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
