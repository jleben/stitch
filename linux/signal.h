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
    void wait() { Reactive::wait(event()); clear(); }
    void clear();

    Event event()
    {
        Event e;
        e.fd = d_fd;
        e.mode = EPOLLIN;
        return e;
    }

    // A Stream and its subscriptions are valid until
    // the Signal that the Stream comes from is destroyed.
    class Stream
    {
        friend class Signal;
        Signal * signal;
        Event_Stream stream;

    public:
        template <typename T>
        void subscribe(T f)
        {
            stream.subscribe([=](){ signal->clear(); f(); });
        }
    };

    Stream stream(Event_Reactor & reactor)
    {
        Stream s;
        s.signal = this;
        s.stream = reactor.add(event());
        return s;
    }

private:
    int d_fd;
};

}

