#pragma once

#include "events.h"

#include <cstdint>

namespace Reactive {

using std::uint32_t;

class File_Event : public Event
{
public:
    enum Type
    {
        Read_Ready,
        Write_Ready
    };

    File_Event(int fd, Type);
    virtual void get_info(int & fd, uint32_t & mode) const override;
    virtual void wait() override;
    virtual void clear() override;

private:
    int d_fd;
    uint32_t d_mode;
};

}
