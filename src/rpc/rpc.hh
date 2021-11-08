#pragma once

#include "raft_types.hh"

#include "proto/message.pb.h"

namespace rpc
{
    class RPC
    {
        public:
            virtual ~RPC() {}

            virtual void send_message(const message::Message& message) = 0;
            virtual std::optional<message::Message> receive_message(raft::node_id_t id) = 0;
    };
}
