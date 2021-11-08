#pragma once

#include <mpi.h>

#include "rpc.hh"

#include "types.hh"
#include "raft_types.hh"
#include "serialization.hh"

namespace mpi
{
    class RPC: public rpc::RPC
    {
        public:
            // Overriden methods
            void send_message(const message::Message& message) override;
            std::optional<message::Message> receive_message(raft::node_id_t id) override;
    };
}
