#include "events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace std;

namespace Stitch {

void wait(const Event & e)
{
    pollfd data;
    data.fd = e.fd;
    data.events = e.poll_events;

    int result;

    do { result = poll(&data, 1, -1); }
    while(result == -1 && errno == EINTR);

    if (result == -1)
        throw std::runtime_error("'poll' failed.");

    e.clear();
}

Event_Reactor::Event_Reactor()
{
    d_epoll_fd = epoll_create(1);

    if (d_epoll_fd == -1)
        throw std::runtime_error("'epoll_create' failed.");

    d_ready_events.resize(5);
}

Event_Reactor::~Event_Reactor()
{
    close(d_epoll_fd);
}

void Event_Reactor::subscribe(const Event & event, Callback cb)
{
    d_watched_events.emplace_back();

    auto & watched_event = d_watched_events.back();
    watched_event.clear = event.clear;
    watched_event.cb = cb;

    epoll_event options;
    options.events = event.epoll_events;
    options.data.ptr = &watched_event;

    if (epoll_ctl(d_epoll_fd, EPOLL_CTL_ADD, event.fd, &options) == -1)
    {
        d_watched_events.pop_back();
        throw std::runtime_error("'epoll_ctl' failed.");
    }
}

void Event_Reactor::run(Mode mode)
{
    d_running = true;

    do
    {
        int timeout = (mode == NoWait) ? 0 : -1;
        int result;

        do {
            result = epoll_wait(d_epoll_fd, d_ready_events.data(), d_ready_events.size(), timeout);
        } while (result == -1 && errno == EINTR);

        if (result < 0)
            throw std::runtime_error("'epoll_wait' failed.");

        for (int i = 0; i < result && d_running; ++i)
        {
            auto & options = d_ready_events[i];
            auto data = reinterpret_cast<Event_Data*>(options.data.ptr);
            data->clear();
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
