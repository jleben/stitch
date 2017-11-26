#include "timer.h"

#include <cstdint>
#include <cstring>

#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Stitch {

Timer::Timer()
{
    d_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (d_fd == -1)
        throw std::runtime_error("'timerfd_create' failed.");
}

Timer::~Timer()
{
    close(d_fd);
}

void Timer::setInterval(const timespec & t, bool repeated)
{
    struct itimerspec spec;

    spec.it_value = t;

    if (repeated)
    {
        spec.it_interval = t;
    }
    else
    {
        spec.it_interval.tv_sec = 0;
        spec.it_interval.tv_nsec = 0;
    }

    if (timerfd_settime(d_fd, 0, &spec, nullptr) != 0)
    {
        throw std::runtime_error(string("Failed to set timer interval: ") + strerror(errno));
    }
}

void Timer::stop()
{
    struct itimerspec spec;

    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;

    if (timerfd_settime(d_fd, 0, &spec, nullptr) != 0)
    {
        throw std::runtime_error(string("Failed to stop timer: ") + strerror(errno));
    }
}

void Timer::clear()
{
    uint64_t count;
    int result;

    do { result = read(d_fd, &count, sizeof(count)); }
    while (result == -1 && errno == EINTR);
}

Event Timer::event()
{
    Event e;
    e.fd = d_fd;
    e.epoll_events = EPOLLIN;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Timer::clear, this);
    return e;
}

}
