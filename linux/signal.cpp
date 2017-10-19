#include "signal.h"

#include <cstdint>
#include <mutex>
#include <list>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Reactive {

Signal::Signal()
{
    d_fd = eventfd(0, EFD_NONBLOCK);
}

Signal::~Signal()
{
    close(d_fd);
}

void Signal::notify()
{
    uint64_t count = 1;
    write(d_fd, &count, sizeof(count));
}

void Signal::clear()
{
    uint64_t count;
    read(d_fd, &count, sizeof(count));
}

Event Signal::event()
{
    Event e;
    e.fd = d_fd;
    e.epoll_events = EPOLLIN;
    e.poll_events = POLLIN;
    return e;
}

}
