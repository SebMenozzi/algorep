#pragma once

#include <iostream> // std::cout
#include <boost/algorithm/string.hpp> // boost::split boost::is_any_of
#include <unistd.h> // sleep

#include "rpc.hh"
#include "raft_clock.hh"
#include "raft_types.hh"
#include "serialization.hh"
#include "types.hh"

// Proto includes
#include "proto/message.pb.h"
#include "proto/command_entry.pb.h"
#include "proto/election_timeout.pb.h"
#include "proto/speed.pb.h"

namespace raft
{
    class Controller
    {
        public:
            Controller(node_id_t id, const std::vector<node_id_t> server_ids, const std::vector<node_id_t> node_ids);
            void set_rpc(class rpc::RPC* rpc) { rpc_ = rpc; }
            void run();
        private:
            void send_command_request(node_id_t id, const std::string& str);
            void send_crash_request(node_id_t id);
            void send_start_request(node_id_t id);
            void send_exit_request(node_id_t id);
            void send_election_timeout_request(node_id_t id, time_t timeout);
            void send_speed_request(node_id_t id, speed::Speed speed);

            speed::Speed string_to_speed(const std::string& str);

            // Id of the controller
            node_id_t id_;
            // Array of server ids
            const std::vector<node_id_t> server_ids_;
            // Array of node ids (server + client)
            const std::vector<node_id_t> node_ids_;
            // Clock to be in sync with clients and servers
            Clock clock_;
        protected:
            rpc::RPC* rpc_ = nullptr;
    };
}
