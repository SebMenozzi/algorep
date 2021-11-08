#pragma once

#include "types.hh"

// Raft types
namespace raft
{
    using node_id_t = uint32;
    using index_t = uint32;
    using term_t = uint32;
    using time_t = sint32;
}
