#include "file_event.h"

#include <cstring>

#include <sys/epoll.h>
#include <poll.h>

using namespace std;

namespace Stitch {

File_Event::File_Event(int fd_, Type type)
{
    fd = fd_;

    switch(type)
    {
    case Read_Ready:
        epoll_events = EPOLLIN;
        poll_events = POLLIN;
        break;
    case Write_Ready:
        epoll_events = EPOLLOUT;
        poll_events = POLLOUT;
        break;
    }

    clear = [](){};
}

}
