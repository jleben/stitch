#include "events_linux.h"
#include "../interface/events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

namespace Concurrency {

class Waiter::Implementation
{
public:
    Implementation()
    {
        epoll_fd = epoll_create(1);
        ready_events.resize(5);
    }

    ~Implementation()
    {
        close(epoll_fd);

        for (Linux_Event * event : watched_events)
        {
            delete event;
        }
    }

    int epoll_fd;
    list<Linux_Event*> watched_events;
    vector<epoll_event> ready_events;
};

Waiter::Waiter():
    d(make_shared<Implementation>())
{}

void Waiter::add_event(Event * event)
{
    auto linux_event = static_cast<Linux_Event*>(event);

    int fd;
    uint32_t mode;
    linux_event->get_info(fd, mode);

    epoll_event options;
    options.events = mode;
    options.data.ptr = linux_event;

    epoll_ctl(d->epoll_fd, EPOLL_CTL_ADD, fd, &options);

    d->watched_events.push_back(linux_event);
}

void Waiter::wait()
{
    int result = epoll_wait(d->epoll_fd, d->ready_events.data(), d->ready_events.size(), -1);

    if (result < 0)
        throw std::runtime_error("Failed epoll_wait.");

    for (int i = 0; i < result; ++i)
    {
        auto & options = d->ready_events[i];
        auto event = reinterpret_cast<Linux_Event*>(options.data.ptr);
        event->happend = true;
    }
}

}
