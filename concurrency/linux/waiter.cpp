#include "waiter.h"

#include <unistd.h>
#include <sys/epoll.h>

#include <cstring>

namespace Stitch {

Waiter::Waiter()
{
    d_fd = epoll_create(1);
}

Waiter::~Waiter()
{
    close(d_fd);
}

void Waiter::add(Event & event)
{
    int fd;
    uint32_t mode;
    event.get_info(fd, mode);

    epoll_event options;
    options.events = mode;
    options.data.ptr = &event;

    int result = epoll_ctl(d_fd, EPOLL_CTL_ADD, fd, &options);
    if (result == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
}

Event * Waiter::next(int timeout)
{
    epoll_event ready;

    int result = epoll_wait(d_fd, &ready, 1, timeout);

    if (result < 0)
        throw std::runtime_error("Failed epoll_wait.");

    if (result < 1)
        return nullptr;

    auto event = reinterpret_cast<Event*>(ready.data.ptr);
    event->clear();

    return event;
}

}
