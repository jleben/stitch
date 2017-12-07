
#include "../stitch/atom_spmc.h"
#include "../stitch/connections.h"
#include "../stitch/signal.h"

#include <memory>

namespace Stitch {

using std::weak_ptr;
using std::shared_ptr;

namespace Detail {

template <typename T> struct VarWriterData;
template <typename T> struct VarReaderData;

template <typename T>
struct VarWriterData
{
    SPMC_Atom<T> value;
    Set<shared_ptr<VarReaderData<T>>> readers;
};

template <typename T> struct VarReaderData
{
    Signal signal;
    weak_ptr<VarWriterData<T>> writer;
};

}

template <typename T> class Variable;
template <typename T> class VariableReader;

template <typename T>
class Variable
{
    friend class VariableReader<T>;

public:
    Variable():
        d(std::make_shared<Detail::VarWriterData<T>>())
    {}

    void set(const T & value)
    {
        d->value.store(value);

        for(auto & reader : d->readers)
        {
            reader->signal.notify();
        }
    }

private:
    shared_ptr<Detail::VarWriterData<T>> d;
};

template <typename T>
class VariableReader
{
public:
    VariableReader(const T & default_value = T()):
        d_default_value(default_value),
        d(std::make_shared<Detail::VarReaderData<T>>())
    {}

    ~VariableReader()
    {
        disconnect();
    }

    void connect(Variable<T> & var)
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

    T get()
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
    shared_ptr<Detail::VarReaderData<T>> d;
};

}
