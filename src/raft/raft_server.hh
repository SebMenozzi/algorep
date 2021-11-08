#pragma once

#include <iostream> // std::cout
#include <cstdlib> // std::rand
#include <ctime> // std::time
#include <unistd.h> // getpid sleep
#include <algorithm> // std::min
#include <string> // std::to_string
#include <queue> // std::queue
#include <google/protobuf/wrappers.pb.h> // google::protobuf::UInt32Value

#include "raft_clock.hh"
#include "raft_storage.hh"
#include "rpc.hh"
#include "raft_types.hh"
#include "types.hh"
#include "serialization.hh"

// Proto includes
#include "proto/append_entry.pb.h"
#include "proto/log_entry.pb.h"
#include "proto/message.pb.h"
#include "proto/vote.pb.h"
#include "proto/search_leader.pb.h"
#include "proto/election_timeout.pb.h"
#include "proto/speed.pb.h"
#include "proto/persistent_state.pb.h"

namespace raft
{
    enum class ServerState { FOLLOWER, CANDIDATE, LEADER, DEAD };

    class Server
    {
        public:
            Server(node_id_t id, node_id_t controller_id, const std::vector<node_id_t> server_ids, const std::vector<node_id_t> node_ids);
            void set_rpc(class rpc::RPC* rpc) { rpc_ = rpc; }
            void run();
        private:
            void restore_state();
            void persist_state();

            void set_election_timeout();
            time_t speed_to_delay();

            void start();
            void crash();

            void handle_state();
            void handle_follower();
            void handle_candidate();
            void handle_leader();

            void leader_send_heartbeats();

            void become_follower(term_t term);
            void become_candidate();
            void become_leader();

            // MARK: - Server and client messages

            void receive_all_messages();
            void handle_messages();
            void handle_vote_request(const message::Message& message);
            void handle_vote_response(const message::Message& message);
            void handle_append_entries_request(const message::Message& message);
            void handle_append_entries_response(const message::Message& message);
            void handle_command_entry_request(const message::Message& message);
            void handle_search_leader_request(const message::Message& message);
            void handle_message(const message::Message& message);

            uint32 apply_new_log_entries(index_t begin_index, std::vector<log_entry::LogEntry>& new_log_entries);
            void check_new_commit_to_apply();

            // MARK: - Controller messages

            void receive_controller_messages();
            void handle_controller_messages();
            void handle_crash_request();
            void handle_start_request();
            void handle_election_timeout_request(const message::Message& message);
            void handle_speed_request(const message::Message& message);
            void handle_controller_message(const message::Message& message);

            // MARK: - Persistent state on all servers

            // Id of the servers
            node_id_t id_;
            // Id of the controller that rules them all
            node_id_t controller_id_;
            // Array of server ids
            const std::vector<node_id_t> server_ids_;
            // Array of all the node ids (client and server)
            const std::vector<node_id_t> node_ids_;
            // Map of server ids with their index
            std::map<node_id_t, index_t> server_indexes_dic_;
            // State of the server (initialized to FOLLOWER on first boot)
            ServerState state_;
            // Clock used for timeout delays (election and heartbeat)
            Clock clock_;
            // Latest term server has seen (initialized to 0 on first boot, increases monotonically)
            term_t current_term_;
            // Election timeout (between 150ms and 300ms)
            time_t election_timeout_;
            // Heartbeat timeout (30-50 ms)
            time_t heartbeat_timeout_;
            // Candidate Id that received vote in current term
            std::optional<node_id_t> voted_for_;
            // Number of votes the candidate received
            uint32 votes_count_;
            // Log entries; each entry contains command for state machine, and term when entry was received by leader (first index is 1)
            std::vector<log_entry::LogEntry> log_entries_;
            // Queue of log entries to commit
            std::queue<log_entry::LogEntry> log_entries_to_commit_;
            // Storage
            Storage storage_;
            // Speed to simulate a delay (for debug purpose only)
            speed::Speed speed_;
            // Queue of messages from other clients and servers
            std::queue<message::Message> messages_;
            // Queue of messages from the controller
            std::queue<message::Message> messages_controller_;
            // Clock used for simulating a delay with servers and clients communication, correlated with speed
            Clock delay_clock_;
            // Is running
            bool running_;

            // MARK: - Volatile state on all servers

            // Index of highest log entry known to be committed (initialized to 0, increase monotonically)
            std::optional<index_t> commit_index_;
            // Index of highest log entry applied to state machine (initialized to 0, increase monotonically)
            std::optional<index_t> last_applied_commit_index_;

            // MARK: - Volatile state on leaders

            // For each server, index of the next log entry to send to that server (initialized to leader last log index + 1)
            std::vector<index_t> next_index_;
            // For each server, index of the highest log entry known to be replicated on server (initialized to 0, increase monotonically)
            std::vector<std::optional<index_t>> match_index_;
        protected:
            rpc::RPC* rpc_ = nullptr;
    };
}
