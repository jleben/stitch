#include "signal.h"

#include <cstdint>
#include <mutex>
#include <list>
#include <stdexcept>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Stitch {

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

Detail::SignalChannel::SignalChannel()
{
    fd = eventfd(0, EFD_NONBLOCK);
    if (fd == -1)
        throw std::runtime_error("'eventfd' failed.");
}

Detail::SignalChannel::~SignalChannel()
{
    close(fd);
}

void Detail::SignalChannel::notify()
{
    uint64_t count = 1;
    int result;

    do { result = write(fd, &count, sizeof(count)); }
    while (result == -1 && errno == EINTR);
}

void Detail::SignalChannel::clear()
{
    uint64_t count;
    int result;

    do { result = read(fd, &count, sizeof(count)); }
    while (result == -1 && errno == EINTR);
}

Event Signal_Receiver::event()
{
    Event e;
    e.fd = data().fd;
    e.epoll_events = EPOLLIN;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Detail::SignalChannel::clear, &data());

    return e;
}


}
