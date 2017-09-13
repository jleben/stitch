#pragma once

#include "../interface/events.h"

namespace Concurrency {

class Linux_Event : public Event
{
public:
    bool happend = false;

    operator bool() override
    {
        return happend;
    }

    virtual void get_info(int & fd, uint32_t & mode) const = 0;
};

}
