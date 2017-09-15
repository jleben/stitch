#include "../interface/file.h"
#include "events_linux.h"

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

namespace Concurrency {

class File_Event : public Linux_Event
{
    int d_fd;
    uint32_t d_mode;

public:
    File_Event(int fd, uint32_t mode):
        d_fd(fd),
        d_mode(mode)
    {}

    void wait() override
    {
        pollfd data;
        data.fd = d_fd;
        data.events = d_mode;

        int result = poll(&data, 1, -1);

        if (result == -1)
            throw std::runtime_error("Failed to wait for event.");
    }

    void get_info(int & fd, uint32_t & mode) const override
    {
        fd = d_fd;
        mode = d_mode;
    }

    void clear() override
    {}
};

class File::Implementation
{
public:
    Implementation(const string & path, File::Access access, bool blocking):
        path(path),
        fd(open_file(path, access)),
        read_event(fd, EPOLLIN),
        write_event(fd, EPOLLOUT)
    {
        if (!blocking)
        {
            int result = fcntl(fd, F_SETFL, O_NONBLOCK);
            if (result == -1)
                throw std::runtime_error("Failed to set non-blocking mode.");
        }
    }

    int open_file(const string & path, File::Access access)
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

        int result = open(path.c_str(), flags);

        if (result < 0)
            throw std::runtime_error(string("Failed to open file: ") + strerror(errno));

        return result;
    }

    string path;
    int fd;
    File_Event read_event;
    File_Event write_event;
};



File::File(const string & path, Access access, bool blocking):
    d(make_shared<Implementation>(path, access, blocking))
{}

File::~File()
{
    close(d->fd);
}

string File::path() const
{
    return d->path;
}

Event & File::read_ready()
{
    return d->read_event;
}

Event & File::write_ready()
{
    return d->write_event;
}

int File::read(void *dst, int count)
{
    // FIXME: report errors

    int read_count = 0;

    while(read_count < count)
    {
        int result = ::read(d->fd, dst, count);
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
        int result = ::write(d->fd, src, count);
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

