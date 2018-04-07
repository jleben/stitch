#pragma once

#include "../stitch/atom.h"
#include "../stitch/signal.h"

#include <memory>

namespace Stitch {

using std::weak_ptr;
using std::shared_ptr;

namespace Detail {

template <typename T> struct State_Data;
template <typename T> struct State_Observer_Data;

template <typename T>
struct State_Data
{
    State_Data() {}
    State_Data(T value): atom(value) {}

    Atom<T> atom;
    Set<shared_ptr<State_Observer_Data<T>>> observers;
};

template <typename T> struct State_Observer_Data
{
    State_Observer_Data(const shared_ptr<State_Data<T>> & state, T value):
        state(state),
        reader(state->atom, value)
    {}

    // NOTE: Mind the order: state must be destoyed after reader.
    shared_ptr<Detail::State_Data<T>> state;
    AtomReader<T> reader;
    Signal signal;
};

}

template <typename T> class State;
template <typename T> class State_Observer;

/*! \brief Stores a value read by connected \ref State_Observer "State_Observers".

  One or more observers can be connected using \ref State_Observer::connect.

  An initial value is stored by the constructors. Following that, a new value
  can be posted using the \ref store method.

  The last stored value is accessed by a connected observer using \ref State_Observer::load.

  Unless otherwise noted, the methods of this class should only be called from a single thread.
 */
template <typename T>
class State
{
    friend class State_Observer<T>;

public:
    /*!
     * \brief Constructs the State and stores a default-constructed value of type T.
     */

    State():
        d_shared(std::make_shared<Detail::State_Data<T>>()),
        d_writer(d_shared->atom)
    {}

    /*!
     * \brief Constructs the State and stores the given value.
     */

    State(const T & value):
        d_shared(std::make_shared<Detail::State_Data<T>>(value)),
        d_writer(d_shared->atom)
    {}


    /*! \brief Returns a reference to the value to be written.

        This value is made available to \ref State_Observer "State_Observers"
        by calling \ref store.

        The returned reference is only valid only the next call to \ref store.
    */

    T & value() { return d_writer.value(); }

    /*! \brief Makes \ref value available to observers and notifies them.
     *
     * Observers are notified via their \ref State_Observer::changed "changed" event.
     */
    void store()
    {
        d_writer.store();

        for(auto & observer : d_shared->observers)
        {
            observer->signal.notify();
        }
    }

    void store(const T & value)
    {
        this->value() = value;
        store();
    }

private:
    shared_ptr<Detail::State_Data<T>> d_shared;
    AtomWriter<T> d_writer;

};

/*! \brief Reads the last value stored by a connected \ref State.

  The observer is connected to a State using \ref connect.

  The method \ref load loads the latest value stored in the State
  and \ref value return a reference to the last loaded value.

  Whenever a new value is stored, the event returned by \ref changed is
  activated.

  Unless otherwise noted, the methods of this class should only be called from a single thread.
*/

template <typename T>
class State_Observer
{
public:
    /*! Constructs the State_Observer with a default value.

      The default value is returned by a call to \ref read
      when the State_Observer is not connected.
    */
    State_Observer(const T & default_value = T()):
        d_default_value(default_value),
        d_current_value(&d_default_value)
    {}

    ~State_Observer()
    {
        disconnect();
    }

    /*! \brief Connects to a \ref State. */
    void connect(State<T> & state)
    {
        disconnect();

        d_shared = std::make_shared<Detail::State_Observer_Data<T>>(state.d_shared, d_default_value);
        state.d_shared->observers.insert(d_shared);

        d_current_value = &d_shared->reader.value();
    }

    /*! \brief If connected to a State, disconnects from it. */
    void disconnect()
    {
        if (!d_shared)
            return;

        d_shared->state->observers.remove(d_shared);
        d_shared.reset();

        d_current_value = &d_default_value;
    }

    /*! \brief Loads the latest value stored by a connected \ref State.

      So long as the State_Observer is connected to a \ref State,
      this method loads the last value stored by the State and
      returns a reference to it.

      When the State_Observer is not connected this method returns
      a reference to the default value passed to the constructor.
     */
    const T & load()
    {
        if (d_shared)
        {
            d_shared->reader.load();
            d_current_value = &d_shared->reader.value();
        }

        return *d_current_value;
    }

    /*! \brief Returns a reference to the last loaded value.
     *
     * The returned reference is only valid until the next call to \ref load.
     */
    const T & value()
    {
        return *d_current_value;
    }

    /*! \brief An \ref Event activated whenever a new value is stored by a connected \ref State. */
    Event changed()
    {
        return d_shared->signal.event();
    }

private:
    T d_default_value;
    shared_ptr<Detail::State_Observer_Data<T>> d_shared { nullptr };
    const T * d_current_value = nullptr;
};

}
