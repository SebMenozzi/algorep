#include "raft_clock.hh"

namespace raft
{
    Clock::Clock()
    {
        reset();
    }

    void Clock::reset()
    {
        start_time_ = get_ticks();
    }

    time_t Clock::get_time()
    {
        return get_ticks() - start_time_;
    }

    time_t Clock::get_ticks()
    {
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);

        time_t seconds_to_milliseconds = tp.tv_sec * 1000;
        long nanoseconds_to_milliseconds = tp.tv_nsec / 1000000;

        return seconds_to_milliseconds + nanoseconds_to_milliseconds;
    }
}
