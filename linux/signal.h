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

    Event_Stream stream(Event_Reactor & reactor)
    {
        return reactor.add(event());
    }

private:
    void clear();

    int d_fd;
};

}

