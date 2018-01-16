#pragma once

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
    NoticeWriterData() {}
    NoticeWriterData(const T & value): value(value) {}

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

/*! \brief Posts a value read by connected \ref NoticeReader "NoticeReaders".

  One or more notice readers can be connected using \ref NoticeReader::connect.

  An initial value is posted by the constructors. Following that, a new value
  can be posted using the \ref post method.

  The last posted value is returned by a call to \ref NoticeReader::read on a connected reader.
  This is true even if it was posted before the reader was connected.

  Unless otherwise noted, the methods of this class should only be called from a single thread.
 */
template <typename T>
class Notice
{
    friend class NoticeReader<T>;

public:
    /*!
     * \brief Constructs the Notice and posts a default-constructed value of type T.
     */

    Notice():
        d(std::make_shared<Detail::NoticeWriterData<T>>())
    {}

    /*!
     * \brief Constructs the Notice and posts a given value.
     */

    Notice(const T & value):
        d(std::make_shared<Detail::NoticeWriterData<T>>(value))
    {}

    /*! \brief Posts a value which is read by connected NoticeReaders.

      \sa \ref NoticeReader::read, \ref NoticeReader::changed.
    */

    void post(const T & value)
    {
        d->value.store(value);

        for(auto & reader : d->readers)
        {
            reader->signal.notify();
        }
    }

    /*! \brief Returns the latest posted value.
     *
     * This can be called from multiple threads.
     */
    T read()
    {
        return d->value.load();
    }

private:
    shared_ptr<Detail::NoticeWriterData<T>> d;
};

/*! \brief Reads the last value posted by a connected \ref Notice.

  The reader is connected to a Notice using \ref connect.

  The posted value is returned by \ref read.

  Whenever a new value is posted, the event returned by \ref changed is
  activated.

  Unless otherwise noted, the methods of this class should only be called from a single thread.
*/

template <typename T>
class NoticeReader
{
public:
    /*! Constructs the NoticeReader with a default value.

      The default value is returned by a call to \ref read
      when the NoticeReader is not connected.
    */
    NoticeReader(const T & default_value = T()):
        d_default_value(default_value),
        d(std::make_shared<Detail::NoticeReaderData<T>>())
    {}

    ~NoticeReader()
    {
        disconnect();
    }

    /*! \brief Connects to a \ref Notice to read the values it posts. */
    void connect(Notice<T> & var)
    {
        disconnect();

        var.d->readers.insert(d);

        d->writer = var.d;
    }

    /*! \brief If connected to a Notice, disconnects from it. */
    void disconnect()
    {
        auto writer = d->writer.lock();
        if (writer)
            writer->readers.remove(d);
        d->writer.reset();
    }

    /*! \brief Returns the latest value posted by a connected \ref Notice.

      So long as the NoticeReader is connected to a \ref Notice,
      this method returns the last value posted on the Notice,
      even if it was posted before the connection.

      When the NoticeReader is not connected this method returns the default
      value passed to the constructor.
     */
    T read()
    {
        auto writer = d->writer.lock();
        if (!writer)
            return d_default_value;
        else
            return writer->value.load();
    }

    /*! \brief An \ref Event activated whenever a new value is posted on a connected \ref Notice. */
    Event changed()
    {
        return d->signal.event();
    }

private:
    T d_default_value;
    shared_ptr<Detail::NoticeReaderData<T>> d;
};

}
