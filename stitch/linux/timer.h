#pragma once

#include "events.h"
#include "utils.h"

#include <memory>

namespace Stitch {

class Timer
{
public:
    Timer();
    ~Timer();

    /**
     * @brief Starts the timer.
     *
     * If `repeated` is true, the \ref event is activated repeatedly
     * with a period equal to `duration`.
     * Otherwise, the event is activated only a single time
     * after `duration`, and then the timer is stopped.
     *
     * When calling this method while the timer is already running,
     * the timer is restarted with the new parameters.
     */
    template<class Rep, class Period>
    void start(const std::chrono::duration<Rep,Period> & duration, bool repeated = false)
    {
        setInterval(to_timespec(duration), repeated);
    }

    /**
     * @brief Stops the timer.
     *
     * After stopping the timer, the \ref event will not be activated
     * unless the timer is started again.
     */

    void stop();

    void wait() { Stitch::wait(event()); }

    /**
     * @brief Returns a momentary event activated every timer period.
     */
    Event event();

private:
    void clear();
    void setInterval(const timespec &, bool repeated = false);

    int d_fd;
};

}

