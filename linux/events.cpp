#include "events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

namespace Reactive {

void Event::subscribe(Event_Reactor & reactor, Callback cb)
{
    reactor.add_event(this, cb);
}

Event_Reactor::Event_Reactor()
{
    d_epoll_fd = epoll_create(1);
    d_ready_events.resize(5);
}

Event_Reactor::~Event_Reactor()
{
    close(d_epoll_fd);
}

void Event_Reactor::add_event(Event * event, Event::Callback cb)
{
    d_watched_events.push_back({ event, cb });

    int fd;
    uint32_t mode;
    event->get_info(fd, mode);

    epoll_event options;
    options.events = mode;
    options.data.ptr = &d_watched_events.back();

    epoll_ctl(d_epoll_fd, EPOLL_CTL_ADD, fd, &options);
}

void Event_Reactor::run(Mode mode)
{
    d_running = true;

    do
    {
        int timeout = (mode == NoWait) ? 0 : -1;
        int result = epoll_wait(d_epoll_fd, d_ready_events.data(), d_ready_events.size(), timeout);

        if (result < 0)
            throw std::runtime_error("Failed epoll_wait.");

        for (int i = 0; i < result; ++i)
        {
            auto & options = d_ready_events[i];
            auto data = reinterpret_cast<Event_Data*>(options.data.ptr);
            data->event->clear();
            if (data->cb)
                data->cb();
        }
    }
    while(mode == WaitUntilQuit && d_running);
}

void Event_Reactor::quit()
{
    d_running = false;
}



}
