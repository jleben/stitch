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

  Progress guarantees use the following parameters:
  - C = Number of connected observers.
  - K = Number of hazard pointers in use.
 */
template <typename T>
class State
{
    friend class State_Observer<T>;

public:
    /*!
     * \brief Constructs the State and stores a default-constructed value of type T.
     *
     * - Progress: Blocking
     * - Time complexity: O(1)
     */

    State():
        d_shared(std::make_shared<Detail::State_Data<T>>()),
        d_writer(d_shared->atom)
    {}

    /*!
     * \brief Constructs the State and stores the given value.
     *
     * - Progress: Blocking
     * - Time complexity: O(1)
     */

    State(const T & value):
        d_shared(std::make_shared<Detail::State_Data<T>>(value)),
        d_writer(d_shared->atom)
    {}

    /*!
     * - Progress: Blocking
     * - Time complexity: O(1)
     */

    ~State() {}

    /*! \brief Returns a reference to the value to be written.

        This value is made available to \ref State_Observer "State_Observers"
        by calling \ref store.

        The returned reference is only valid only the next call to \ref store.

       - Progress: Wait-free
       - Time complexity: O(1)
    */

    T & value() { return d_writer.value(); }

    /*! \brief Makes \ref value available to observers and notifies them.
     *
     * Observers are notified via their \ref State_Observer::changed "changed" event.
     *
     * - Progress: Lock-free.
     * - Time complexity: O(C + K).
     */
    void store()
    {
        d_writer.store();

        for(auto & observer : d_shared->observers)
        {
            observer->signal.notify();
        }
    }

    /*! \brief Copies given value to \ref value and makes it available to observers.
     *
     * This is equivalent to:
     *
     *     observer.value() = value;
     *     observer.store();
     *
     * - Progress: Lock-free.
     * - Time complexity: O(C + K).
     */
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
  and \ref value returns a reference to the last loaded value.

  Whenever a new value is stored, the event returned by \ref changed is
  activated.

  Unless otherwise noted, the methods of this class should only be called from a single thread.

  Progress guarantees use the following parameters:
  - C = Number of connected observers.
  - H = Maximum allowable number of hazard pointers.
*/

template <typename T>
class State_Observer
{
public:
    /*! Constructs the observer with a default value.

      The default value is returned by \ref load and \ref value
      when the observer is not connected.

      - Progress: Blocking.
      - Time complexity: O(1).
    */
    State_Observer(const T & default_value = T()):
        d_default_value(default_value),
        d_shared(std::make_shared<Detail::State_Observer_Data<T>>()),
        d_current_value(&d_default_value)
    {}

    /*!
      - Progress: Blocking.
      - Time complexity: Asymptotic O(C). Worst-case O(C + H).
     */
    ~State_Observer()
    {
        disconnect();
    }

    /*! \brief Connects to a \ref State.
     *
     * - Progress: Blocking.
     * - Time complexity: O(C).
     */

    void connect(State<T> & state)
    {
        disconnect();

        d_state = state.d_shared;
        d_reader = std::make_shared<AtomReader<T>>(d_state->atom, d_default_value);
        d_state->observers.insert(d_shared);

        d_current_value = &d_reader->value();
    }

    /*! \brief If connected to a State, disconnects from it.

      - Progress: Blocking.
      - Time complexity: Asymptotic O(C). Worst-case O(C + H).
     */
    void disconnect()
    {
        if (!d_state)
            return;

        // Break connection
        d_state->observers.remove(d_shared);

        // Delete reader and possibly state data
        d_reader.reset();
        d_state.reset();

        // Restore default value as current value
        d_current_value = &d_default_value;
    }

    /*! \brief Loads the latest value stored by a connected \ref State.

      So long as the observer is connected to a \ref State,
      this method loads the last value stored by the State and
      returns a reference to it.

      When the observer is not connected this method returns
      a reference to the default value passed to the constructor.

      - Progress: Wait-free.
      - Time complexity: O(1).
     */
    const T & load()
    {
        if (d_reader)
        {
            d_reader->load();
            d_current_value = &d_reader->value();
        }

        return *d_current_value;
    }

    /*! \brief Returns a reference to the last loaded value.
     *
     * The returned reference is only valid until the next call to \ref load.
     *
     * - Progress: Wait-free.
     * - Time complexity: O(1).
     */
    const T & value()
    {
        return *d_current_value;
    }

    /*! \brief An \ref Event activated whenever a new value is stored by a connected \ref State.
     *
     * - Progress: Wait-free.
     * - Time complexity: O(1).
     */
    Event changed()
    {
        return d_shared->signal.event();
    }

private:
    T d_default_value;
    // NOTE: Mind the order: state must be destoyed after reader.
    shared_ptr<Detail::State_Observer_Data<T>> d_shared { nullptr };
    shared_ptr<Detail::State_Data<T>> d_state { nullptr };
    shared_ptr<AtomReader<T>> d_reader { nullptr };

    const T * d_current_value = nullptr;
};

}
