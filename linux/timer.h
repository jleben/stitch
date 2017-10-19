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

    void wait() { Reactive::wait(event()); clear(); }
    void clear();

    Event event();

    class Stream
    {
        friend class Timer;
        Timer * timer;
        Event_Stream stream;

    public:
        template <typename T>
        void subscribe(T f)
        {
            auto t = timer;
            // FIXME: Not cool: every subscriber will clear.
            stream.subscribe([=](){ t->clear(); f(); });
        }
    };

    Stream stream(Event_Reactor & reactor)
    {
        Stream s;
        s.timer = this;
        s.stream = reactor.add(event());
        return s;
    }

private:
    void setInterval(const timespec &, bool repeated = false);

    int d_fd;
};

}

