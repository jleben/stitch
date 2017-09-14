#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace Concurrency {

using std::function;

class Waiter;
class Linux_Event;

class Event
{
public:
    using Callback = function<void()>;

    virtual ~Event() {}
    virtual void wait() = 0;
    virtual void subscribe(Waiter*, Callback) = 0;
    virtual void clear() = 0;
};

class Waiter
{
public:
    Waiter();
    void wait();

    class Implementation;

private:
    friend class Linux_Event;

    std::shared_ptr<Implementation> d;
};

}
