#pragma once

#include <time.h>

#include "types.hh"

namespace raft
{
    class Clock
    {
        public:
            Clock();
            void reset();
            time_t get_time();
            time_t get_ticks(void);
        private:
            time_t start_time_;
    };
}
