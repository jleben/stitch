#pragma once

#include "../interface/events.h"

#include <list>
#include <vector>

#include <sys/epoll.h>

namespace Concurrency {

using std::list;
using std::vector;

class Linux_Event : public Event
{
public:
    void subscribe(Event_Reactor&, Callback cb) override;

    virtual void get_info(int & fd, uint32_t & mode) const = 0;
};

class Event_Reactor::Implementation
{
public:

    struct Event_Data
    {
        Linux_Event * event;
        Event::Callback cb;
    };

    Implementation();
    ~Implementation();

    void add_event(Linux_Event * event, Event::Callback cb);
    void run(Event_Reactor::Mode m);

    bool running = false;

    int epoll_fd;

    list<Event_Data> watched_events;
    vector<epoll_event> ready_events;
};

}
