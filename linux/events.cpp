#include "events_linux.h"
#include "../interface/events.h"

#include <algorithm>

#include <sys/select.h>
#include <cstring>

using namespace std;

namespace Concurrency {

Waiter::Waiter(const std::vector<Event*> & events):
    d_events(events)
{}

Event * Waiter::wait()
{
    fd_set fds;
    FD_ZERO(&fds);

    int fd_count = 0;

    for (auto event : d_events)
    {
        int fd = event->fd();
        FD_SET(fd, &fds);
        fd_count = max(fd + 1, fd_count);
    }

    int result = select(fd_count, &fds, nullptr, nullptr, nullptr);

    if (result < 0)
    {
        throw std::runtime_error(string("Select failed: ") + strerror(errno));
    }

    if (result == 0)
        return nullptr;

    for (auto event : d_events)
    {
        if (FD_ISSET(event->fd(), &fds))
        {
            event->clear();
            return event;
        }
    }

    throw std::runtime_error("Selected file descriptor not found.");
}

}
