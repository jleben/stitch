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

void Signal::wait()
{
    pollfd data;
    data.fd = d_fd;
    data.events = POLLIN;

    int result = poll(&data, 1, -1);

    if (result == -1)
        throw std::runtime_error("Failed to wait for event.");

    clear();
}

void Signal::get_info(int & fd, uint32_t & mode) const
{
    fd = d_fd;
    mode = EPOLLIN;
}

void Signal::clear()
{
    uint64_t count;
    read(d_fd, &count, sizeof(count));
}

}
