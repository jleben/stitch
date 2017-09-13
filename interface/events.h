#pragma once

#include <memory>
#include <vector>

namespace Concurrency {

class Event
{
public:
    virtual ~Event() {}
    virtual operator bool() = 0;
    virtual void clear() = 0;
};

class Waiter
{
public:
    Waiter();
    void add_event(Event*);
    void wait();

    class Implementation;

private:
    std::shared_ptr<Implementation> d;
};

}
