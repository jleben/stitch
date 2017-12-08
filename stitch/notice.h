
#include "../stitch/atom_spmc.h"
#include "../stitch/connections.h"
#include "../stitch/signal.h"

#include <memory>

namespace Stitch {

using std::weak_ptr;
using std::shared_ptr;

namespace Detail {

template <typename T> struct NoticeWriterData;
template <typename T> struct NoticeReaderData;

template <typename T>
struct NoticeWriterData
{
    SPMC_Atom<T> value;
    Set<shared_ptr<NoticeReaderData<T>>> readers;
};

template <typename T> struct NoticeReaderData
{
    Signal signal;
    weak_ptr<NoticeWriterData<T>> writer;
};

}

template <typename T> class Notice;
template <typename T> class NoticeReader;

template <typename T>
class Notice
{
    friend class NoticeReader<T>;

public:
    Notice():
        d(std::make_shared<Detail::NoticeWriterData<T>>())
    {}

    void post(const T & value)
    {
        d->value.store(value);

        for(auto & reader : d->readers)
        {
            reader->signal.notify();
        }
    }

private:
    shared_ptr<Detail::NoticeWriterData<T>> d;
};

template <typename T>
class NoticeReader
{
public:
    NoticeReader(const T & default_value = T()):
        d_default_value(default_value),
        d(std::make_shared<Detail::NoticeReaderData<T>>())
    {}

    ~NoticeReader()
    {
        disconnect();
    }

    void connect(Notice<T> & var)
    {
        disconnect();

        var.d->readers.insert(d);

        d->writer = var.d;
    }

    void disconnect()
    {
        auto writer = d->writer.lock();
        if (writer)
            writer->readers.remove(d);
        d->writer.reset();
    }

    T read()
    {
        auto writer = d->writer.lock();
        if (!writer)
            return d_default_value;
        else
            return writer->value.load();
    }

    Event changed()
    {
        return d->signal.event();
    }

private:
    T d_default_value;
    shared_ptr<Detail::NoticeReaderData<T>> d;
};

}
