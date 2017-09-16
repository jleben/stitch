#pragma once

#include "events.h"

#include <memory>

namespace Reactive {

class Signal : public Event
{
public:
    Signal();
    ~Signal();
    void notify();
    virtual void get_info(int & fd, uint32_t & mode) const override;
    virtual void wait() override;
    virtual void clear() override;

private:
    int d_fd;
};

}

