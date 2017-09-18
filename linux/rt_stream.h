#pragma once

#include <atomic>
#include <vector>
#include <mutex>
#include <memory>

namespace Reactive {

using std::atomic;
using std::vector;
using std::mutex;
using std::lock_guard;
using std::shared_ptr;

template <typename T>
class Realtime_Stream_Source;

template <typename T>
class Realtime_Stream_Sink;

template <typename T>
void connect(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink, int);

template <typename T>
void disconnect(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink);

template <typename T>
class RT_Stream
{
    friend class Realtime_Stream_Source<T>;
    friend class Realtime_Stream_Sink<T>;

public:
    RT_Stream(int size): d_data(size) {}

    int size() const { return int(d_data.size()); }

    int readable_size() const
    {
        int s = size();

        if (!s)
            return 0;

        int r = d_read_pos;
        int w = d_write_pos;
        int count = (s + w - r) % s;
        return count;
    }

    int writable_size() const
    {
        int s = size();

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

private:
    atomic<int> d_write_pos { 0 };
    atomic<int> d_read_pos { 0 };
    vector<T> d_data;
};

template <typename T>
class Stream_Iterator
{
    friend class Realtime_Stream_Source<T>;
    friend class Realtime_Stream_Sink<T>;

    T * d;
    int size;
    int pos;
    Stream_Iterator() {}

public:
    Stream_Iterator & operator++()
    {
        pos = (pos + 1) % size;
    }

    T & operator*()
    {
        return d[pos];
    }

    bool operator!=(const Stream_Iterator & other)
    {
        return pos != other.pos;
    }
};

template <typename T>
class Realtime_Stream_Source
{
public:
    friend class Realtime_Stream_Sink<T>;

    friend
    void connect<T>(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink, int);

    friend
    void disconnect<T>(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink);

    ~Realtime_Stream_Source()
    {
        bool done = false;

        while(!done)
        {
            lock_guard<mutex> guard(d_mutex);

            if (d_sink && d_sink->d_mutex.try_lock())
            {
                d_sink->d_source = nullptr;
                d_sink->d_stream = nullptr;
                d_sink = nullptr;
            }

            done = !d_sink;
        }
    }

    void push(const T & v)
    {
        auto stream = d_stream;
        if (!stream || !stream->writable_size())
            throw std::runtime_error("No space to write.");
        stream->d_data[stream->d_write_pos] = v;
        stream->advance_write(1);
    }

    struct Range
    {
        Range(shared_ptr<RT_Stream<T>> s, int size):
            stream(s),
            size(size)
        {}

        ~Range()
        {
            stream->advance_write(size);
        }

        shared_ptr<RT_Stream<T>> stream;
        int size;

        Stream_Iterator<T> begin()
        {
            Stream_Iterator<T> it;
            it.d = stream->d_data.data();
            it.size = stream->size();
            it.pos = stream->d_write_pos;
            return it;
        }

        Stream_Iterator<T> end()
        {
            Stream_Iterator<T> it;
            it.pos = (stream->d_write_pos + size) % stream->size();
            return it;
        }
    };

    Range range() const
    {
        auto stream = d_stream;
        return Range(stream, stream->writable_size());
    }

    Range range(int size) const
    {
        auto stream = d_stream;
        if (size > stream->writable_size())
            throw std::runtime_error("Not enough space.");
        return Range(stream, size);
    }

private:
    mutex d_mutex;
    shared_ptr<RT_Stream<T>> d_stream = nullptr;
    Realtime_Stream_Sink<T> * d_sink = nullptr;
};

template <typename T>
class Realtime_Stream_Sink
{
public:
    friend class Realtime_Stream_Source<T>;

    friend
    void connect<T>(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink, int);

    friend
    void disconnect<T>(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink);

    ~Realtime_Stream_Sink()
    {
        bool done = false;

        while(!done)
        {
            lock_guard<mutex> guard(d_mutex);

            if (d_source && d_source->d_mutex.try_lock())
            {
                d_source->d_sink = nullptr;
                d_source->d_stream = nullptr;
                d_source = nullptr;
            }

            done = !d_source;
        }
    }

    T pop()
    {
        auto stream = d_stream;
        if (!stream || !stream->readable_size())
            throw std::runtime_error("Nothing to read.");
        T v = stream->d_data[stream->d_read_pos];
        stream->advance_read(1);
        return v;
    }

    void clear()
    {
        auto stream = d_stream;
        if (!stream)
            return;
        stream->advance_read(stream->readable_size());
    }

    struct Range
    {
        Range(shared_ptr<RT_Stream<T>> s, int size):
            stream(s),
            size(size)
        {}

        ~Range()
        {
            stream->advance_read(size);
        }

        shared_ptr<RT_Stream<T>> stream;
        int size;

        Stream_Iterator<T> begin()
        {
            Stream_Iterator<T> it;
            it.d = stream->d_data.data();
            it.size = stream->size();
            it.pos = stream->d_read_pos;
            return it;
        }

        Stream_Iterator<T> end()
        {
            Stream_Iterator<T> it;
            it.pos = (stream->d_read_pos + size) % stream->size();
            return it;
        }
    };

    Range range() const
    {
        auto stream = d_stream;
        return Range(stream, stream->readable_size());
    }

    Range range(int size) const
    {
        auto stream = d_stream;

        if (size > stream->readable_size())
            throw std::runtime_error("Not enough data available.");

        return Range(d_stream, size);
    }

private:
    mutex d_mutex;
    shared_ptr<RT_Stream<T>> d_stream = nullptr;
    Realtime_Stream_Source<T> * d_source = nullptr;
};

template <typename T>
void connect(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink, int size)
{
    lock_guard<mutex> gp(source.d_mutex);
    lock_guard<mutex> gc(sink.d_mutex);

    if (source.d_stream || sink.d_stream)
    {
        if (source.d_stream == sink.d_stream)
            return;
        else
            throw std::runtime_error("Already connected elsewhere.");
    }

    auto stream = std::make_shared<RT_Stream<T>>(size);
    source.d_sink = &sink;
    sink.d_source = &source;
    source.d_stream = stream;
    sink.d_stream = stream;
}

template <typename T>
void disconnect(Realtime_Stream_Source<T> & source, Realtime_Stream_Sink<T> & sink)
{
    lock_guard<mutex> gp(source.d_mutex);
    lock_guard<mutex> gc(sink.d_mutex);

    if (source.d_stream != nullptr && source.d_stream == sink.d_stream)
    {
        source.d_sink = nullptr;
        sink.d_source = nullptr;
        source.d_stream = nullptr;
        sink.d_stream = nullptr;
    }
}

}
