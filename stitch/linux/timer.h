#pragma once

#include "events.h"
#include "utils.h"

#include <memory>

namespace Stitch {

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

    void wait() { Stitch::wait(event()); }

    Event event();

private:
    void clear();
    void setInterval(const timespec &, bool repeated = false);

    int d_fd;
};

}

