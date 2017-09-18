#pragma once

#include <mutex>
#include <queue>
#include <list>
#include <queue>
#include <algorithm>

namespace Reactive {

using std::mutex;
using std::lock_guard;
using std::list;
using std::queue;

template <typename T>
class Stream_Source;

template <typename T>
class Stream_Sink;

template <typename T>
void connect(Stream_Source<T> & source, Stream_Sink<T> & sink);

template <typename T>
void disconnect(Stream_Source<T> & source, Stream_Sink<T> & sink);

template <typename T>
bool are_connected(Stream_Source<T> & source, Stream_Sink<T> & sink);

template <typename T>
class Stream_Source
{
public:
    friend class Stream_Sink<T>;
    friend void connect<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);
    friend void disconnect<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);
    friend bool are_connected<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);

    ~Stream_Source()
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
            sink->push(val);
        }
    }

    bool has_connections()
    {
        lock_guard<mutex> guard(d_mutex);
        return !d_sinks.empty();
    }

private:
    mutex d_mutex;
    list<Stream_Sink<T>*> d_sinks;
};

template <typename T>
class Stream_Sink
{
public:
    friend class Stream_Source<T>;
    friend void connect<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);
    friend void disconnect<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);
    friend bool are_connected<T>(Stream_Source<T> & source, Stream_Sink<T> & sink);

    Stream_Sink() {}

    Stream_Sink(int capacity):
        d_capacity(capacity)
    {
        if (capacity < 1)
            throw std::runtime_error("Invalid capacity.");
    }

    ~Stream_Sink()
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
                it = d_sources.erase(it);
            }

            done = d_sources.empty();
        }
    }

    bool empty()
    {
        lock_guard<mutex> guard(d_mutex);
        return d_queue.empty();
    }

    T pop()
    {
        lock_guard<mutex> guard(d_mutex);
        auto v = d_queue.front();
        d_queue.pop();
        return v;
    }

    bool has_connections()
    {
        lock_guard<mutex> guard(d_mutex);
        return !d_sources.empty();
    }

private:
    void push(const T & val)
    {
        lock_guard<mutex> guard(d_mutex);
        if (d_queue.size() == d_capacity)
            throw std::runtime_error("Out of space.");

        d_queue.push(val);
    }

    friend class Stream_Source<T>;

    mutex d_mutex;
    queue<T> d_queue;
    int d_capacity = -1;
    list<Stream_Source<T>*> d_sources;
};

template <typename T>
inline
void connect(Stream_Source<T> & source, Stream_Sink<T> & sink)
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
void disconnect(Stream_Source<T> & source, Stream_Sink<T> & sink)
{
    lock_guard<mutex> guard1(source.d_mutex);
    lock_guard<mutex> guard2(sink.d_mutex);

    source.d_sinks.remove(&sink);
    sink.d_sources.remove(&source);
}

template <typename T>
inline
bool are_connected(Stream_Source<T> & source, Stream_Sink<T> & sink)
{
    lock_guard<mutex> guard1(source.d_mutex);
    lock_guard<mutex> guard2(sink.d_mutex);

    return find(source.d_sinks.begin(), source.d_sinks.end(), &sink) != source.d_sinks.end();
}

}
