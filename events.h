#pragma once

#include <memory>
#include <vector>

namespace Concurrency {

class Event;

class Waiter
{
public:
    Waiter(const std::vector<Event*> & events);
    Event * wait();

private:
    std::vector<Event*> d_events;
};

}
