#include "../interface/timer.h"
#include "events_linux.h"

#include <cstdint>
#include <cstring>

#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Concurrency {

class Timer_Event : public Linux_Event
{
    int d_fd;

public:
    Timer_Event()
    {
        d_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    }

    int fd() { return d_fd; }

    void wait() override
    {
        pollfd data;
        data.fd = d_fd;
        data.events = POLLIN;

        int result = poll(&data, 1, -1);

        if (result == -1)
            throw std::runtime_error("Failed to wait for event.");

        clear();
    }

    void get_info(int & fd, uint32_t & mode) const override
    {
        fd = d_fd;
        mode = EPOLLIN;
    }

    void clear() override
    {
        uint64_t count;
        read(d_fd, &count, sizeof(count));
    }
};

class Timer::Implementation
{
public:
    Timer_Event event;
};

Timer::Timer():
    d(make_shared<Implementation>())
{

}

Event * Timer::event()
{
    return &d->event;
}

void Timer::setInterval(int seconds, bool repeated)
{
    int fd = d->event.fd();

    struct itimerspec spec;

    spec.it_value.tv_sec = seconds;
    spec.it_value.tv_nsec = 0;

    if (repeated)
    {
        spec.it_interval.tv_sec = seconds;
        spec.it_interval.tv_nsec = 0;
    }
    else
    {
        spec.it_interval.tv_sec = 0;
        spec.it_interval.tv_nsec = 0;
    }

    if (timerfd_settime(fd, 0, &spec, nullptr) != 0)
    {
        throw std::runtime_error(string("Failed to set timer interval: ") + strerror(errno));
    }
}

}
