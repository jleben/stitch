#include "timer.h"

#include <cstdint>
#include <cstring>

#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Reactive {

Timer::Timer()
{
    d_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
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

void Timer::get_info(int & fd, uint32_t & mode) const
{
    fd = d_fd;
    mode = EPOLLIN;
}

void Timer::wait()
{
    pollfd data;
    data.fd = d_fd;
    data.events = POLLIN;

    int result = poll(&data, 1, -1);

    if (result == -1)
        throw std::runtime_error("Failed to wait for event.");

    clear();
}

void Timer::clear()
{
    uint64_t count;
    read(d_fd, &count, sizeof(count));
}

}
