#include "file.h"

#include <cstdint>
#include <mutex>
#include <list>
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

File::File(const string & path, File::Access access, bool blocking):
    File(open_file(path, access, blocking))
{}

File::File(int fd):
    d_fd(fd),
    d_read_ready(fd, File_Event::Read_Ready),
    d_write_ready(fd, File_Event::Write_Ready)
{}

File::~File()
{
    close(d_fd);
}

int File::open_file(const string & path, File::Access access, bool blocking)
{
    int flags;

    switch (access)
    {
    case ReadOnly:
        flags = O_RDONLY;
        break;
    case WriteOnly:
        flags = O_WRONLY;
        break;
    case ReadWrite:
        flags = O_RDWR;
        break;
    }

    int fd;

    {
        int result = open(path.c_str(), flags);

        if (result < 0)
            throw std::runtime_error(string("Failed to open file: ") + strerror(errno));

        fd = result;
    }

    if (!blocking)
    {
        int result = fcntl(fd, F_SETFL, O_NONBLOCK);
        if (result == -1)
            throw std::runtime_error("Failed to set non-blocking mode.");
    }

    return fd;
}

int File::read(void *dst, int count)
{
    // FIXME: report errors

    int read_count = 0;

    while(read_count < count)
    {
        int result = ::read(d_fd, dst, count);
        if (result == -1)
        {
            if (errno != EINTR)
                break;
        }
        else if (result > 0)
        {
            read_count += result;
            dst = (char*)(dst) + result;
        }
        else
        {
            break;
        }
    }

    return read_count;
}

int File::write(void *src, int count)
{
    // FIXME: report errors

    int write_count = 0;

    while(write_count < count)
    {
        int result = ::write(d_fd, src, count);
        if (result == -1)
        {
            if (errno != EINTR)
                break;
        }
        else if (result > 0)
        {
            write_count += result;
            src = (char*)src + result;
        }
        else
        {
            break;
        }
    }

    return write_count;
}

}

