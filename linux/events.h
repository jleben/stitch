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
    using Callback = function<void()>;

    virtual ~Event() {}
    virtual void get_info(int & fd, uint32_t & mode) const = 0;
    virtual void wait() = 0;
    virtual void clear() = 0;

    void subscribe(Event_Reactor&, Callback);
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

    void run(Mode = NoWait);
    void quit();

private:
    friend class Event;

    struct Event_Data
    {
        Event * event;
        Event::Callback cb;
    };

    void add_event(Event * event, Event::Callback cb);

    bool d_running = false;

    int d_epoll_fd;

    list<Event_Data> d_watched_events;
    vector<epoll_event> d_ready_events;
};

}
