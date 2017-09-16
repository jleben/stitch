#pragma once

#include <chrono>
#include <ratio>

#include <time.h>
#include <sys/time.h>

namespace Reactive {

template<class Rep, class Period>
inline
timeval to_timeval(const std::chrono::duration<Rep,Period> & d)
{
    using namespace std;
    using namespace chrono;

    auto usecs = duration_cast<duration<uint64_t,micro>>(d);

    timeval t;
    if (usecs.count() <= 0)
    {
        t.tv_sec = 0;
        t.tv_usec = 0;
    }
    else
    {
        t.tv_sec = usecs.count() / std::micro::den;
        t.tv_usec = usecs.count() % std::micro::den;
    }

    return t;
}

template<class Rep, class Period>
inline
timespec to_timespec(const std::chrono::duration<Rep,Period> & d)
{
    using namespace std;
    using namespace chrono;

    auto nsecs = duration_cast<duration<uint64_t,nano>>(d);

    timespec t;
    if (nsecs.count() <= 0)
    {
        t.tv_sec = 0;
        t.tv_nsec = 0;
    }
    else
    {
        t.tv_sec = nsecs.count() / std::nano::den;
        t.tv_nsec = nsecs.count() % std::nano::den;
    }

    return t;
}

}
