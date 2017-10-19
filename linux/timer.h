#pragma once

#include "events.h"
#include "utils.h"

#include <memory>

namespace Reactive {

class Timer
{
public:
    Timer();
    ~Timer();

    template<class Rep, class Period>
    void start(const std::chrono::duration<Rep,Period> & dur, bool repeated = false)
    {
        setInterval(to_timespec(dur), repeated);
    }

    void stop();

    void wait() { Reactive::wait(event()); }

    Event event();

    Event_Stream stream(Event_Reactor & reactor)
    {
        return reactor.add(event());
    }

private:
    void clear();
    void setInterval(const timespec &, bool repeated = false);

    int d_fd;
};

}

