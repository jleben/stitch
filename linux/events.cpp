#include "events_linux.h"
#include "../interface/events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

namespace Concurrency {

void Linux_Event::subscribe(Event_Reactor & reactor, Callback cb)
{
    reactor.d->add_event(this, cb);
}

Event_Reactor::Event_Reactor():
    d(make_shared<Implementation>())
{}

void Event_Reactor::run(Mode mode)
{
    d->run(mode);
}

void Event_Reactor::quit()
{
    d->running = false;
}

Event_Reactor::Implementation::Implementation()
{
    epoll_fd = epoll_create(1);
    ready_events.resize(5);
}

Event_Reactor::Implementation::~Implementation()
{
    close(epoll_fd);
}

void Event_Reactor::Implementation::add_event(Linux_Event * event, Event::Callback cb)
{
    watched_events.push_back({ event, cb });

    int fd;
    uint32_t mode;
    event->get_info(fd, mode);

    epoll_event options;
    options.events = mode;
    options.data.ptr = &watched_events.back();

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &options);
}

void Event_Reactor::Implementation::run(Mode m)
{
    running = true;

    do
    {
        int timeout = m == NoWait ? 0 : -1;
        int result = epoll_wait(epoll_fd, ready_events.data(), ready_events.size(), timeout);

        if (result < 0)
            throw std::runtime_error("Failed epoll_wait.");

        for (int i = 0; i < result; ++i)
        {
            auto & options = ready_events[i];
            auto data = reinterpret_cast<Implementation::Event_Data*>(options.data.ptr);
            data->event->clear();
            if (data->cb)
                data->cb();
        }
    }
    while(m == WaitUntilQuit && running);
}

}
