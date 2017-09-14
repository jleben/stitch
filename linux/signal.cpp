#include "../interface/signal.h"
#include "events_linux.h"

#include <cstdint>
#include <mutex>
#include <list>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Concurrency {

class Signal_Event : public Linux_Event
{
    int d_fd;
    weak_ptr<Signal::Implementation> d_parent;

public:
    Signal_Event(shared_ptr<Signal::Implementation> parent):
        d_parent(parent)
    {
        d_fd = eventfd(0, EFD_NONBLOCK);
    }

    ~Signal_Event();

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

class Signal::Implementation
{
public:
    std::mutex mutex;
    list<Signal_Event*> events;
};

Signal::Signal():
    d(make_shared<Implementation>())
{}

Event * Signal::event()
{
    auto event = new Signal_Event(d);

    {
        lock_guard<mutex> guard(d->mutex);
        d->events.push_back(event);
    }

    return event;
}

void Signal::notify()
{
    {
        lock_guard<mutex> guard(d->mutex);

        uint64_t count = 1;

        for (auto event : d->events)
        {
            write(event->fd(), &count, sizeof(count));
        }
    }
}

Signal_Event::~Signal_Event()
{
    auto parent = d_parent.lock();

    {
        lock_guard<mutex> guard(parent->mutex);
        parent->events.remove(this);
    }

    close(d_fd);
}

}
