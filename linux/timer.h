#pragma once

#include "events.h"
#include "utils.h"

#include <memory>

namespace Reactive {

class Timer : public Event
{
public:
    Timer();
    ~Timer();

    template<class Rep, class Period>
    void start(const std::chrono::duration<Rep,Period> & dur, bool repeated = false)
    {
        setInterval(to_timespec(dur), repeated);
    }

    void stop();

    void wait() override;
    void clear() override;

    int fd() const { return d_fd; }
    void get_info(int & fd, uint32_t & mode) const override;

private:
    void setInterval(const timespec &, bool repeated = false);

    int d_fd;
};

}

