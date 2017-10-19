#pragma once

#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/epoll.h>

namespace Reactive {

using std::function;
using std::list;
using std::vector;

class Event_Reactor;

class Event
{
public:
    int fd;
    uint32_t epoll_events;
    short poll_events;
};

// FIXME: Reactive::wait is not very useful on its own,
// because user must manually clear Signal, Timer...
void wait(const Event &);

/*
"Stream" concept:

A class with:
 - template <typename T> void subscribe(T handler)

Should not cause undefined behavior when destroyed after subscribing.
Therfore, a stream derived form another stream should probably have that
stream as a member.
*/

/*
Class Event_Stream:
Satisfies the "Stream" concept.
Can be used until the Event_Reactor that it comes from is destroyed.
*/

class Event_Stream
{
    friend class Event_Reactor;
    using Callback = function<void()>;

public:
    template <typename T>
    void subscribe(T handler)
    {
        callbacks->push_back(handler);
    }

private:
    list<Callback> * callbacks;
};

class Event_Reactor
{
public:
    enum Mode
    {
        NoWait,
        Wait,
        WaitUntilQuit
    };

    Event_Reactor();
    ~Event_Reactor();

    Event_Stream add(const Event & event);
    void run(Mode = NoWait);
    void quit();

private:
    friend class Event;
    friend class Event_Stream;

    struct Event_Data
    {
        list<Event_Stream::Callback> cb;
    };

    bool d_running = false;

    int d_epoll_fd;

    list<Event_Data> d_watched_events;
    vector<epoll_event> d_ready_events;
};

}
