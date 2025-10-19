#include "file.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>

using namespace std;

namespace Stitch {

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
    int flags = 0;

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
    int read_count = 0;

    while(read_count < count)
    {
        int result = ::read(d_fd, dst, count);
        if (result == -1)
        {
            switch(errno)
            {
            case EAGAIN:
                break;
            case EINTR:
                continue;
            default:
                throw std::runtime_error(string("'read' failed: ") + strerror(errno));
            }
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
    int write_count = 0;

    while(write_count < count)
    {
        int result = ::write(d_fd, src, count);
        if (result == -1)
        {
            switch(errno)
            {
            case EAGAIN:
                break;
            case EINTR:
                continue;
            default:
                throw std::runtime_error(string("'read' failed: ") + strerror(errno));
            }
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

