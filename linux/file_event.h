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

    File_Event(int fd, Type type);
};

}
