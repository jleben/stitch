#pragma once

#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/epoll.h>

namespace Stitch {

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
    function<void()> clear;
};

/*! \brief Wait for a single event. */
void wait(const Event &);

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

    void subscribe(const Event & event, Callback);
    void run(Mode = NoWait);
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
