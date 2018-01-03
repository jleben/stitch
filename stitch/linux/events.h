#pragma once

#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/epoll.h>
#include <poll.h>

namespace Stitch {

using std::function;
using std::list;
using std::vector;

class Event_Reactor;

/*! \brief Generic representation of all kinds of event sources.

There are two basic kinds of events, both of which are represented
by the Event class:
- Momentary: Representing momentary occurrences, such as an elapsed point in time.
- Conditional: Representing an ongoing condition, such as a FIFO file having data to read.

An event can be in one of two states: active or inactive.
Methods like \ref wait and \ref Event_Reactor::run, which are
called "handlers", observe and modify this state.

A momentary event becomes active when the underlying moment occurs (e.g. a timer expires),
and is immediately deactivated by a handler.
A conditional event becomes active when the underlying condition begins
(e.g. a FIFO file becomes ready to read)
and is deactivated by handlers only after the condition ends
(e.g. all data in the file is read and there is no more data to read).
*/

class Event
{
public:
    int fd;
    uint32_t epoll_events;
    short poll_events;
    function<void()> clear;
};

/*! \brief Waits for a single event to become active.

If the event is momentary, it is deactivated.

\sa
- \ref Event, for details on event kinds and  states.
- \ref Event_Reactor, for waiting on multiple events with callbacks.
*/

void wait(const Event &);

/*! \brief Waits for one of multiple events to become active.

Momentary events are deactivated before returning.

This function can be called with a brace-enclosed list:

    wait({ event1, event2 });

\sa
- \ref Event, for details on event kinds and  states.
- \ref Event_Reactor, for waiting on multiple events with callbacks.
*/

// FIXME: Test needed for the following function.

template <size_t N>
void wait(const Event (&events)[N])
{
    pollfd data[N];
    for (int i = 0; i < N; ++i)
    {
        data[i].fd = events[i].fd;
        data[i].events = events[i].poll_events;
    }

    int result;

    do { result = poll(data, N, -1); }
    while(result == -1 && errno == EINTR);

    if (result == -1)
        throw std::runtime_error("'poll' failed.");

    for (int i = 0; i < N; ++i)
    {
        events[i].clear();
    }
}

/*! \brief Waits for multiple events to become active and invokes subscribed callbacks.

\sa
- \ref Event, for details on event kinds and  states.
- \ref wait, for waiting on a single event without a callback.
*/

class Event_Reactor
{
public:
    using Callback = function<void()>;

    enum Mode
    {
        NoWait,
        Wait,
        WaitUntilQuit
    };

    Event_Reactor();
    ~Event_Reactor();

    /*! \brief Subscribes `callback` to be invoked when `event` is activated.
     *
     * It is safe to throw an exception from the callback. See \ref run for details.
    */
    void subscribe(const Event & event, Callback callback);

    /*! \brief Monitors event activations and invokes subscribed callbacks.

      The \ref Mode parameter has the following effect:

      - NoWait: Handles each currently active event once and returns without waiting at all.
      - Wait: If no event with a subscription is currently active,
        waits until at least one is activated, handles each active event once and returns.
      - WaitUntilQuit: Keeps handling all active events until \ref quit is called,
        at which point it returns immediately.

      This method is "fair" in the following sense.
      It handles each active event once
      before handling the same event again.

      "Handling" an active event means invoking the subscribed callback
      and deactivating momentary events.

      It is safe to throw an exception from a callback. Exceptions are
      simply propagated by the `run` method.
      Apart from that, throwing an exception has the same effect
      as calling \ref quit from a callback.

      \sa \ref Event, for details on event kinds and  states.
    */
    void run(Mode mode = NoWait);

    /*! \brief Signals the ongoing \ref run method to terminate.
     *
     * When called from a callback during an execution of \ref run,
     * makes the latter return immediately after the callback returns.
     *
     * When called otherwise, it has no effect.
     *
     * Note that this class is not thread-safe, which means that the only way
     * to call this method during an execution of \ref run is from within
     * a callback.
     */
    void quit();

private:
    friend class Event;
    friend class Event_Stream;

    struct Event_Data
    {
        function<void()> clear;
        Callback cb;
    };

    bool d_running = false;

    int d_epoll_fd;

    list<Event_Data> d_watched_events;
    vector<epoll_event> d_ready_events;
};

}
