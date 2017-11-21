#pragma once

#include "mpsc_queue.h"
#include "lockfree_set.h"

#include <mutex>
#include <list>
#include <algorithm>
#include <memory>

namespace Reactive {

using std::mutex;
using std::lock_guard;
using std::list;
using std::shared_ptr;

namespace Detail {

template <typename T>
struct Stream_Producer_Data;

template <typename T>
struct Stream_Consumer_Data;

template <typename T>
using Stream_Consumer_Data_Ptr = shared_ptr<Stream_Consumer_Data<T>>;

template <typename T>
using Stream_Producer_Data_Ptr = shared_ptr<Stream_Producer_Data<T>>;

template <typename T>
struct Stream_Producer_Data
{
    Set<Stream_Consumer_Data_Ptr<T>> sinks;
};

template <typename T>
struct Stream_Consumer_Data
{
    Stream_Consumer_Data(int capacity): queue(capacity) {}

    Set<Stream_Producer_Data_Ptr<T>> sources;
    MPSC_Queue<T> queue;
};

}

template <typename T>
class Stream_Producer;

template <typename T>
class Stream_Consumer;

template <typename T>
void connect(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

template <typename T>
void disconnect(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

template <typename T>
bool are_connected(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

template <typename T>
class Stream_Producer
{
    using Consumer_Ptr = Detail::Stream_Consumer_Data_Ptr<T>;

public:
    friend class Stream_Consumer<T>;
    friend void connect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend void disconnect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend bool are_connected<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

    // Blocking

    Stream_Producer(): d(new Detail::Stream_Producer_Data<T>) {}

    // Blocking

    ~Stream_Producer()
    {
        d->sinks.for_each([&](const Consumer_Ptr & consumer){
            consumer->sources.remove(d);
        });
    }

    // Lock-free

    void push(const T & val)
    {
        d->sinks.for_each([&](const Consumer_Ptr & consumer){
            consumer->queue.push(val);
        });
    }

    // Lock-free

    bool has_connections()
    {
        return !d->sinks.empty();
    }

private:
    Detail::Stream_Producer_Data_Ptr<T> d;
};

template <typename T>
class Stream_Consumer
{
    using Producer_Ptr = Detail::Stream_Producer_Data_Ptr<T>;

public:
    friend class Stream_Producer<T>;
    friend void connect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend void disconnect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend bool are_connected<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

    // Blocking

    Stream_Consumer(int capacity): d(new Detail::Stream_Consumer_Data<T>(capacity))
    {
        if (capacity < 1)
            throw std::runtime_error("Invalid capacity.");
    }

    // Blocking

    ~Stream_Consumer()
    {
        d->sources.for_each([&](const Producer_Ptr & producer){
            producer->sinks.remove(d);
        });
    }

    // Wait-free

    bool empty()
    {
        return d->queue.empty();
    }

    // Wait-free

    bool pop(T & v)
    {
        return d->queue.pop(v);
    }

    // Lock-free

    bool has_connections()
    {
        return !d->sources.empty();
    }

private:
    Detail::Stream_Consumer_Data_Ptr<T> d;
};

// Thread safe against:
// - Itself
// - All methods of Stream_Producer<T> and Stream_Consumer<T> other than the destructors.
// Blocking.
// Time O(N).

template <typename T>
inline
void connect(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    source.d->sinks.insert(sink.d);
    sink.d->sources.insert(source.d);
}

// Thread safe against:
// - Itself
// - All methods of Stream_Producer<T> and Stream_Consumer<T> other than the destructors.
// Blocking.
// Time O(N).

template <typename T>
inline
void disconnect(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    source.d->sinks.remove(sink.d);
    sink.d->sources.remove(source.d);
}

// Thread safe against:
// - Itself
// - All methods of Stream_Producer<T> and Stream_Consumer<T> other than the destructors.
// Lock-free.
// Time O(N).

template <typename T>
inline
bool are_connected(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    return source.d->sinks.contains(sink.d);
}

}
