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
class Stream_Producer;

template <typename T>
class Stream_Consumer;

template <typename T>
void connect(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer, int);

template <typename T>
void disconnect(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer);

template <typename T>
class Stream
{
    friend class Stream_Producer<T>;
    friend class Stream_Consumer<T>;

public:
    Stream(int size): d_data(size) {}

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
    friend class Stream_Producer<T>;
    friend class Stream_Consumer<T>;

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
class Stream_Producer
{
public:
    friend
    void connect<T>(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer, int);

    friend
    void disconnect<T>(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer);

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
        Range(shared_ptr<Stream<T>> s, int size):
            stream(s),
            size(size)
        {}

        ~Range()
        {
            stream->advance_write(size);
        }

        shared_ptr<Stream<T>> stream;
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
    shared_ptr<Stream<T>> d_stream = nullptr;
};

template <typename T>
class Stream_Consumer
{
public:
    friend
    void connect<T>(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer, int);

    friend
    void disconnect<T>(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer);

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
        Range(shared_ptr<Stream<T>> s, int size):
            stream(s),
            size(size)
        {}

        ~Range()
        {
            stream->advance_read(size);
        }

        shared_ptr<Stream<T>> stream;
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
    shared_ptr<Stream<T>> d_stream = nullptr;
};

template <typename T>
void connect(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer, int size)
{
    lock_guard<mutex> gp(producer.d_mutex);
    lock_guard<mutex> gc(consumer.d_mutex);

    if (producer.d_stream || consumer.d_stream)
    {
        if (producer.d_stream == consumer.d_stream)
            return;
        else
            throw std::runtime_error("Already connected elsewhere.");
    }

    auto stream = std::make_shared<Stream<T>>(size);
    producer.d_stream = stream;
    consumer.d_stream = stream;
}

template <typename T>
void disconnect(Stream_Producer<T> & producer, Stream_Consumer<T> & consumer)
{
    lock_guard<mutex> gp(producer.d_mutex);
    lock_guard<mutex> gc(consumer.d_mutex);

    if (producer.d_stream != nullptr && producer.d_stream == consumer.d_stream)
    {
        producer.d_stream = nullptr;
        consumer.d_stream = nullptr;
    }
}

}
