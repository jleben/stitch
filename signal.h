#pragma once

#include "events.h"

#include <memory>

namespace Concurrency {

class Signal
{
public:
    Signal();
    void notify();
    Event * event();

    class Implementation;
private:
    std::shared_ptr<Implementation> d;
};

}
