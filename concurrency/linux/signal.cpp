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
    if (d_fd == -1)
        throw std::runtime_error("'eventfd' failed.");
}

Signal::~Signal()
{
    close(d_fd);
}

void Signal::notify()
{
    uint64_t count = 1;
    int result;

    do { result = write(d_fd, &count, sizeof(count)); }
    while (result == -1 && errno == EINTR);
}

void Signal::clear()
{
    uint64_t count;
    int result;

    do { result = read(d_fd, &count, sizeof(count)); }
    while (result == -1 && errno == EINTR);
}

Event Signal::event()
{
    Event e;
    e.fd = d_fd;
    e.epoll_events = EPOLLIN;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Signal::clear, this);
    return e;
}

}
