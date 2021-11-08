#pragma once

#include <iostream> // std::cout
#include <queue> // std::queue
#include <fstream> // std::ifstream std::getline
#include <unistd.h> // sleep

#include "raft_clock.hh"
#include "rpc.hh"
#include "raft_types.hh"
#include "serialization.hh"
#include "types.hh"

// Proto includes
#include "proto/message.pb.h"
#include "proto/command_entry.pb.h"
#include "proto/search_leader.pb.h"

namespace raft
{
    enum class ClientState { ALIVE, DEAD };

    class Client
    {
        public:
            Client(node_id_t id, node_id_t controller_id, const std::vector<node_id_t> server_ids);
            void set_rpc(class rpc::RPC* rpc) { rpc_ = rpc; }
            void run();
        private:
            void start();
            void crash();
            void populate_commands_from_file(const std::string& path);

            // MARK: - Server messages

            void receive_servers_messages();

            void handle_search_leader_response(const message::Message& message);
            void handle_command_entry_response(const message::Message& message);
            void handle_server_message(const message::Message& message);

            // MARK: - Controller messages

            void receive_controller_messages();

            void handle_crash_request();
            void handle_start_request();
            void handle_command_entry_request(const message::Message& message);
            void handle_controller_message(const message::Message& message);

            // MARK: - Leader methods

            void search_leader();
            void reset_leader();

            // MARK: - Command methods

            void send_next_command();
            void check_command_timeout();

            // Id of the client
            node_id_t id_;
            // Id of the controller that rules them all
            node_id_t controller_id_;
            // Array of server ids
            const std::vector<node_id_t> server_ids_;
            // State of the client (initialized to DEAD on first boot)
            ClientState state_;
            // Leader timeout (30-50 ms)
            time_t timeout_;
            // Clock used for leader timeout
            Clock leader_clock_;
            // Clock used for command timeout
            Clock command_clock_;
            // Leader Id
            std::optional<node_id_t> leader_id_;
            // Queue of commands to send to the leader
            std::queue<std::string> commands_to_send_;
            // False when the next command in the queue is not committed on the leader
            bool next_command_sent_;
            // Is running
            bool running_;
        protected:
            rpc::RPC* rpc_ = nullptr;
    };
}
