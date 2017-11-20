#pragma once

#include "mpsc_queue.h"

#include <mutex>
#include <list>
#include <algorithm>

namespace Reactive {

using std::mutex;
using std::lock_guard;
using std::list;

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
public:
    friend class Stream_Consumer<T>;
    friend void connect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend void disconnect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend bool are_connected<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

    ~Stream_Producer()
    {
        bool done = false;

        while(!done)
        {
            lock_guard<mutex> guard(d_mutex);

            auto it = d_sinks.begin();
            while(it != d_sinks.end())
            {
                auto sink = *it;
                if (!sink->d_mutex.try_lock())
                    break;
                sink->d_sources.remove(this);
                sink->d_mutex.unlock();

                it = d_sinks.erase(it);
            }

            done = d_sinks.empty();
        }
    }

    void push(const T & val)
    {
        lock_guard<mutex> guard(d_mutex);

        for (auto sink : d_sinks)
        {
            sink->d_queue.push(val);
        }
    }

    bool has_connections()
    {
        lock_guard<mutex> guard(d_mutex);
        return !d_sinks.empty();
    }

private:
    mutex d_mutex;
    list<Stream_Consumer<T>*> d_sinks;
};

template <typename T>
class Stream_Consumer
{
public:
    friend class Stream_Producer<T>;
    friend void connect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend void disconnect<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);
    friend bool are_connected<T>(Stream_Producer<T> & source, Stream_Consumer<T> & sink);

    Stream_Consumer(int capacity):
        d_queue(capacity)
    {
        if (capacity < 1)
            throw std::runtime_error("Invalid capacity.");
    }

    ~Stream_Consumer()
    {
        bool done = false;

        while(!done)
        {
            lock_guard<mutex> guard(d_mutex);

            auto it = d_sources.begin();
            while(it != d_sources.end())
            {
                auto src = *it;
                if (!src->d_mutex.try_lock())
                    break;
                src->d_sinks.remove(this);
                src->d_mutex.unlock();

                it = d_sources.erase(it);
            }

            done = d_sources.empty();
        }
    }

    bool empty()
    {
        return d_queue.empty();
    }

    bool pop(T & v)
    {
        return d_queue.pop(v);
    }

    bool has_connections()
    {
        lock_guard<mutex> guard(d_mutex);
        return !d_sources.empty();
    }

private:
    friend class Stream_Producer<T>;

    mutex d_mutex;
    list<Stream_Producer<T>*> d_sources;
    MPSC_Queue<T> d_queue;
};

template <typename T>
inline
void connect(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    lock_guard<mutex> guard1(source.d_mutex);
    lock_guard<mutex> guard2(sink.d_mutex);

    bool connected = find(source.d_sinks.begin(), source.d_sinks.end(), &sink) != source.d_sinks.end();
    if (connected)
        return;

    source.d_sinks.push_back(&sink);
    sink.d_sources.push_back(&source);
}

template <typename T>
inline
void disconnect(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    lock_guard<mutex> guard1(source.d_mutex);
    lock_guard<mutex> guard2(sink.d_mutex);

    source.d_sinks.remove(&sink);
    sink.d_sources.remove(&source);
}

template <typename T>
inline
bool are_connected(Stream_Producer<T> & source, Stream_Consumer<T> & sink)
{
    lock_guard<mutex> guard1(source.d_mutex);
    lock_guard<mutex> guard2(sink.d_mutex);

    return find(source.d_sinks.begin(), source.d_sinks.end(), &sink) != source.d_sinks.end();
}

}
