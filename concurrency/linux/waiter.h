#pragma once

#include "events.h"

namespace Stitch {

class Waiter
{
public:
    Waiter();
    ~Waiter();
    void add(Event &);
    Event * next() { return next(0); }
    Event * wait() { return next(-1); }
private:
    Event * next(int timeout);

    int d_fd;
};

}
