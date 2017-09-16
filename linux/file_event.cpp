#include "file_event.h"

#include <cstring>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>

using namespace std;

namespace Reactive {

File_Event::File_Event(int fd, Type type):
    d_fd(fd)
{
    switch(type)
    {
    case Read_Ready:
        d_mode = EPOLLIN;
        break;
    case Write_Ready:
        d_mode = EPOLLOUT;
        break;
    }
}

void File_Event::get_info(int & fd, uint32_t & mode) const
{
    fd = d_fd;
    mode = d_mode;
}

void File_Event::wait()
{
    pollfd data;
    data.fd = d_fd;
    data.events = d_mode;

    int result = poll(&data, 1, -1);

    if (result == -1)
        throw std::runtime_error("Failed to wait for event.");
}

void File_Event::clear()
{}

}
