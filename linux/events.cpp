#include "events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace std;

namespace Reactive {

void wait(const Event & e)
{
    pollfd data;
    data.fd = e.fd;
    data.events = e.mode;

    int result = poll(&data, 1, -1);

    if (result == -1)
        throw std::runtime_error("Failed to wait for event.");
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

Event_Stream Event_Reactor::add(const Event & event)
{
    d_watched_events.emplace_back();

    epoll_event options;
    options.events = event.mode;
    options.data.ptr = &d_watched_events.back();

    epoll_ctl(d_epoll_fd, EPOLL_CTL_ADD, event.fd, &options);

    Event_Stream stream;
    stream.callbacks = &d_watched_events.back().cb;
    return stream;
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
            for(auto & cb : data->cb)
                cb();
        }
    }
    while(mode == WaitUntilQuit && d_running);
}

void Event_Reactor::quit()
{
    d_running = false;
}

}
