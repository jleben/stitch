#pragma once

#include "events.h"

#include <memory>

namespace Concurrency {

class Timer
{
public:
    Timer();
    template <typename D>
    void setInterval(D duration);

    void setInterval(int seconds, bool repeated = false);

    void setRepated(bool);

    Event * event();

    class Implementation;
private:
    std::shared_ptr<Implementation> d;
};

}

