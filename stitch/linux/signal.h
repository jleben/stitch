#pragma once

#include "events.h"

#include <memory>

namespace Stitch {

class Signal
{
public:
    Signal();
    ~Signal();

    void notify();
    void wait() { Stitch::wait(event()); }

    Event event();

private:
    void clear();

    int d_fd;
};

}

