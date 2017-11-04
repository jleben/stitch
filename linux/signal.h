#pragma once

#include "events.h"

#include <memory>

namespace Reactive {

class Signal
{
public:
    Signal();
    ~Signal();

    void notify();
    void wait() { Reactive::wait(event()); }

    Event event();

private:
    void clear();

    int d_fd;
};

}

