#pragma once

#include "events.h"

namespace Concurrency {

class Event
{
public:
    // Disconnect from source
    virtual ~Event() {}

    // Get file descriptor for waiting
    virtual int fd() const = 0;

    virtual void clear() = 0;
};

}
