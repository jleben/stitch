#pragma once

#include "events.h"

#include <memory>

namespace Reactive {

class Timer : public Event
{
public:
    Timer();
    ~Timer();

    void setInterval(int seconds, bool repeated = false);
    void wait() override;
    void clear() override;

    int fd() const { return d_fd; }
    void get_info(int & fd, uint32_t & mode) const override;

private:
    int d_fd;
};

}

