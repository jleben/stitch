#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace Concurrency {

using std::function;

class Event_Reactor;
class Linux_Event;

class Event
{
public:
    using Callback = function<void()>;

    virtual ~Event() {}
    virtual void wait() = 0;
    virtual void subscribe(Event_Reactor&, Callback) = 0;
    virtual void clear() = 0;
};

class Event_Reactor
{
public:
    Event_Reactor();

    enum Mode
    {
        NoWait,
        Wait,
        WaitUntilQuit
    };

    void run(Mode = NoWait);
    void quit();

    class Implementation;

private:
    friend class Linux_Event;

    std::shared_ptr<Implementation> d;
};

}
