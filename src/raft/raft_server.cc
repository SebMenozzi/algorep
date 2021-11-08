#include "raft_server.hh"

namespace raft
{
    // MARK: - Public

    Server::Server(
        node_id_t id,
        node_id_t controller_id,
        const std::vector<node_id_t> server_ids,
        const std::vector<node_id_t> node_ids
    ):
        id_(id),
        controller_id_(controller_id),
        server_ids_(server_ids),
        node_ids_(node_ids),
        state_(ServerState::DEAD),
        clock_(),
        current_term_(0),
        heartbeat_timeout_(50),
        voted_for_(std::nullopt),
        votes_count_(0),
        log_entries_(),
        storage_(id),
        speed_(speed::Speed::NONE),
        delay_clock_(),
        running_(true),
        commit_index_(std::nullopt),
        last_applied_commit_index_(std::nullopt),
        next_index_(server_ids.size(), 0),
        match_index_(server_ids.size(), std::nullopt)
    {
        for (index_t i = 0; i < server_ids_.size(); ++i)
            server_indexes_dic_[server_ids_.at(i)] = i;

        set_election_timeout();

        // Restore previous state if it exists
        if (storage_.has_data())
            restore_state();
    }

    void Server::run()
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " running..." << std::endl;
        #endif

        while (running_)
        {
            receive_controller_messages();
            handle_controller_messages();

            if (state_ != ServerState::DEAD)
            {
                receive_all_messages();

                if (delay_clock_.get_time() >= speed_to_delay())
                {
                    delay_clock_.reset();
                    handle_messages();
                }

                check_new_commit_to_apply();
                handle_state();
            }
        }

        #ifdef DEBUG
        std::cout << "Server " << id_ << " is stopping..." << std::endl;
        #endif

        sleep(1);
    }

    // MARK: - Private

    void Server::restore_state()
    {
        persistent_state::PersistentState state = storage_.get();

        current_term_ = state.current_term();
        voted_for_ = state.has_voted_for() ? std::make_optional(state.voted_for().value()) : std::nullopt;

        std::vector<log_entry::LogEntry> log_entries;
        for (index_t i = 0; i < (index_t) state.log_entries_size(); ++i)
        {
            auto log_entry = state.log_entries(i);
            log_entries.push_back(log_entry);
        }
        log_entries_ = log_entries;

        #ifdef DEBUG
        std::cout << "Restore state from server " << id_ << '\n'
                  << "- Current term: " << current_term_ << '\n'
                  << "- Voted for: " << (voted_for_.has_value() ? (int) voted_for_.value() : -1) << '\n'
                  << "- Number of logs: " << log_entries_.size() << std::endl;
        #endif
    }

    void Server::persist_state()
    {
        persistent_state::PersistentState state;
        state.set_current_term(current_term_);
        if (voted_for_.has_value())
        {
            google::protobuf::UInt32Value* voted_for = google::protobuf::UInt32Value().New();
            voted_for->set_value(voted_for_.value());
            state.set_allocated_voted_for(voted_for);
        }

        // TODO: @sebmenozzi instead of saving the all log entries that is inefficient,
        // we would create a log compaction mechanism (section 7)
        for (auto entry = log_entries_.begin(); entry != log_entries_.end(); ++entry)
        {
            log_entry::LogEntry* new_entry = state.add_log_entries();
            new_entry->set_client_id(entry->client_id());
            new_entry->set_leader_id(entry->leader_id());
            new_entry->set_index(entry->index());
            new_entry->set_command(entry->command());
            new_entry->set_term(entry->term());
        }

        storage_.save(state);
    }

    void Server::set_election_timeout()
    {
        uint16 min = 150;
        uint16 max = 300;

        // Define a unique seed for each process
        std::srand(std::time(nullptr) + getpid() + id_);
        // Random delay between 150ms and 300ms
        election_timeout_ = std::rand() % (max - min + 1) + min;
    }

    time_t Server::speed_to_delay()
    {
        switch (speed_)
        {
            case speed::Speed::LOW:
                return 50;
            case speed::Speed::MEDIUM:
                return 25;
            case speed::Speed::HIGH:
                return 10;
            default:
                return 0;
        }
    }

    // Run Server loop
    void Server::start()
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " started!" << std::endl;
        #endif

        state_ = ServerState::FOLLOWER;
    }

    void Server::crash()
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " crashed!" << std::endl;
        #endif

        // Clear the messages queue
        while(!messages_.empty())
            messages_.pop();

        // Clear log entries to commit queue
        while (!log_entries_to_commit_.empty())
            log_entries_to_commit_.pop();

        // Reset server
        state_ = ServerState::DEAD;
    }

    // Handle state of the server
    void Server::handle_state()
    {
        switch (state_)
        {
            case ServerState::FOLLOWER:
                handle_follower();
                break;
            case ServerState::CANDIDATE:
                handle_candidate();
                break;
            case ServerState::LEADER:
                handle_leader();
                break;
            default:
                break;
        }
    }

    // If the Server is a follower
    void Server::handle_follower()
    {
        if (clock_.get_time() > election_timeout_)
            become_candidate();
    }

    // If the Server is a candidate
    void Server::handle_candidate()
    {
        // If the server didn't receive the majority of vote to become a leader after the election delay, restart the election
        if (clock_.get_time() > election_timeout_)
            become_candidate();
    }

    // Leader: Send a Append Entries request to followers
    void Server::leader_send_heartbeats()
    {
        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::APPEND_ENTRIES_REQUEST);
        message.set_term(current_term_);

        for (const auto& id: server_ids_)
        {
            if (id != id_) // Exclude self
            {
                append_entry::AppendEntriesRequest request;

                index_t idx = server_indexes_dic_[id];
                index_t next_index = next_index_.at(idx);

                // Only send logs from the next index
                if (next_index < log_entries_.size())
                {
                    for (auto entry = log_entries_.begin() + next_index; entry != log_entries_.end(); ++entry)
                    {
                        log_entry::LogEntry* new_entry = request.add_log_entries();
                        new_entry->set_client_id(entry->client_id());
                        new_entry->set_leader_id(entry->leader_id());
                        new_entry->set_index(entry->index());
                        new_entry->set_command(entry->command());
                        new_entry->set_term(entry->term());
                    }
                }

                request.set_term(current_term_);
                request.set_leader_id(id_);

                // If a previous log exist, then add metadata in proto
                std::optional<index_t> prev_log_index = next_index == 0 ? std::nullopt : std::make_optional(next_index - 1);

                if (prev_log_index && prev_log_index.value() >= 0 && prev_log_index.value() < log_entries_.size())
                {
                    index_t index = prev_log_index.value();

                    append_entry::PrevLogMetadata* prev_log_metadata = append_entry::PrevLogMetadata().New();
                    prev_log_metadata->set_prev_log_index(index);
                    prev_log_metadata->set_prev_log_term(log_entries_.at(index).term());
                    request.set_allocated_prev_log_metadata(prev_log_metadata);
                }

                if (commit_index_)
                {
                    google::protobuf::UInt32Value* leader_commit_index = google::protobuf::UInt32Value().New();
                    leader_commit_index->set_value(commit_index_.value());
                    request.set_allocated_leader_commit_index(leader_commit_index);
                }

                message.set_source_id(id_);
                message.set_dest_id(id);
                message.mutable_payload()->PackFrom(request);
                rpc_->send_message(message);
            }
        }

        clock_.reset();
    }

    void Server::handle_leader()
    {
        // Send a Append Entry request every heartbeat timeout to prevent election timeouts to followers
        if (clock_.get_time() >= heartbeat_timeout_)
            leader_send_heartbeats();
    }

    // Change server state to follower
    void Server::become_follower(term_t term)
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " becomes follower" << std::endl;
        #endif

        state_ = ServerState::FOLLOWER;
        current_term_ = term;
        votes_count_ = 0;
        voted_for_ = std::nullopt;

        clock_.reset();
    }

    // Change server state to candidate
    void Server::become_candidate()
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " becomes candidate" << std::endl;
        #endif

        state_ = ServerState::CANDIDATE;
        voted_for_ = std::make_optional(id_); // Vote for self
        votes_count_ = 1; // Increment vote count
        current_term_ += 1; // Increment current term

        #ifdef DEBUG
        std::cout << "Current Term: " << current_term_ << std::endl;
        #endif

        // Reset election timer
        clock_.reset();
        // For the next term, reset the timeout
        set_election_timeout();

        // Send vote request to all other servers
        vote::VoteRequest request;
        request.set_candidate_id(id_);

        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::VOTE_REQUEST);
        message.mutable_payload()->PackFrom(request);
        message.set_term(current_term_);

        // Ask every servers to vote for us
        for (const auto& id: server_ids_)
        {
            if (id != id_) // Exclude self
            {
                message.set_dest_id(id);
                rpc_->send_message(message);
            }
        }
    }

    // Change server state to leader
    void Server::become_leader()
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " becomes leader" << std::endl;
        #endif

        state_ = ServerState::LEADER;

        for (const auto& id: server_ids_)
        {
            // Retrieve index for the server
            index_t server_index = server_indexes_dic_[id];

            next_index_.at(server_index) = log_entries_.size();
            match_index_.at(server_index) = std::nullopt;
        }

        // Send initial AppendEntries request to each follower
        leader_send_heartbeats();
    }

    void Server::handle_messages()
    {
        if (!messages_.empty())
        {
            message::Message message = messages_.front();
            handle_message(message);
            messages_.pop();
        }
    }

    // Server receives a vote request
    void Server::handle_vote_request(const message::Message& message)
    {
        // Outdated term
        if (message.term() > current_term_)
            become_follower(message.term());

        vote::VoteRequest request;
        message.payload().UnpackTo(&request);

        // Send Vote Response
        vote::VoteResponse response;

        // If votedFor is null or candidatedId, and candidate's log is at least as up-to-date as receiver's log, grant vote
        if (
            message.term() == current_term_ &&
            (voted_for_ == std::nullopt || voted_for_.value() == request.candidate_id())
        )
        {
            response.set_vote_granted(true);
            voted_for_ = std::make_optional(request.candidate_id());

            clock_.reset();
        }
        else // Reply false if term < currentTerm
            response.set_vote_granted(false);

        message::Message response_message;
        response_message.set_source_id(id_);
        response_message.set_dest_id(request.candidate_id());
        response_message.set_type(message::MessageType::VOTE_RESPONSE);
        response_message.mutable_payload()->PackFrom(response);
        response_message.set_term(current_term_);

        rpc_->send_message(response_message);

        persist_state();
    }

    // Server receives a vote response
    void Server::handle_vote_response(const message::Message& message)
    {
        if (state_ != ServerState::CANDIDATE)
            return;

        vote::VoteResponse response;
        message.payload().UnpackTo(&response);

        votes_count_ += response.vote_granted() ? 1 : 0;

        if (response.vote_granted())
        {
            #ifdef DEBUG
            std::cout << "Server " << id_ << " has " << votes_count_ << " vote(s)!" << std::endl;
            #endif
        }

        // If votes received from majority of servers: become leader
        if (votes_count_ >= server_ids_.size() / 2 + 1)
            become_leader();
    }

    // Apply new log_entries
    uint32 Server::apply_new_log_entries(index_t begin_index, std::vector<log_entry::LogEntry>& new_log_entries)
    {
        uint32 count = 0;

        index_t old_log_index = begin_index;
        index_t new_log_index = 0;

        bool is_conflicted = false;

        while (!is_conflicted)
        {
            if (
                (old_log_index >= log_entries_.size()) ||
                (new_log_index >= new_log_entries.size())
            ) {
                break;
            }

            auto old_log_term = log_entries_.at(old_log_index).term();
            auto new_log_term = new_log_entries.at(new_log_index).term();

            if (old_log_term != new_log_term)
            {
                is_conflicted = true;
                break;
            }

            ++old_log_index;
            ++new_log_index;
        }

        if (is_conflicted)
        {
            log_entries_.erase(log_entries_.end() - old_log_index, log_entries_.end());

            #ifdef DEBUG
            std::cout << "Server " << id_ << " had conflicted log entries from index " << old_log_index << "!" << std::endl;
            #endif
        }

        for (index_t i = new_log_index; i < new_log_entries.size(); ++i)
        {
            log_entries_.push_back(new_log_entries.at(i));
            ++count;
        }

        return count;
    }

    // Server receives an Append Entries request
    // i.e message from the leader to followers to apply its log entries
    void Server::handle_append_entries_request(const message::Message& message)
    {
        clock_.reset();

        // Outdated term or not a follower (only one leader can exist)
        if (message.term() > current_term_ || state_ != ServerState::FOLLOWER)
            become_follower(message.term());

        append_entry::AppendEntriesRequest request;
        message.payload().UnpackTo(&request);

        // Send Append Entries Response
        append_entry::AppendEntriesResponse response;

        if (message.term() == current_term_)
        {
            index_t prev_log_index = request.prev_log_metadata().prev_log_index();
            index_t prev_log_term = request.prev_log_metadata().prev_log_term();

            if (
                !request.has_prev_log_metadata() ||
                (request.has_prev_log_metadata() && prev_log_index < log_entries_.size() && prev_log_term == log_entries_.at(prev_log_index).term())
            )
            {
                response.set_success(true);
                response.set_nb_log_entries(request.log_entries_size());

                // Retrieve log_entries in the response
                std::vector<log_entry::LogEntry> new_log_entries;
                for (index_t i = 0; i < (index_t) request.log_entries_size(); ++i)
                    new_log_entries.push_back(request.log_entries(i));

                // Apply new log_entries
                index_t begin_index = !request.has_prev_log_metadata() ? 0 : prev_log_index + 1;
                uint32 nb_of_new_logs = apply_new_log_entries(begin_index, new_log_entries);

                #ifdef DEBUG
                if (nb_of_new_logs > 0)
                    std::cout << "Server " << id_ << " has applied " << nb_of_new_logs << " log(s)" << std::endl;
                #endif

                // If leaderCommit > commitIndex, set commitIndex=min(leaderCommit, index of last new entry)
                if (
                    (!commit_index_ && request.has_leader_commit_index()) ||
                    (request.has_leader_commit_index() && request.leader_commit_index().value() > commit_index_.value())
                )
                {
                    index_t commit_index = std::min(
                        (int) request.leader_commit_index().value(),
                        std::max(0, (int) log_entries_.size() - 1)
                    );

                    commit_index_ = std::make_optional(commit_index);
                }
            }
            else // Reply false if log_entries don't contain an entry at prevLogIndex whose term matches pervLogTerm
                response.set_success(false);
        }
        else // Reply false if term < currentTerm
            response.set_success(false);

        message::Message response_message;
        response_message.set_source_id(id_);
        response_message.set_dest_id(request.leader_id());
        response_message.set_type(message::MessageType::APPEND_ENTRIES_RESPONSE);
        response_message.mutable_payload()->PackFrom(response);
        response_message.set_term(current_term_);

        if (request.log_entries_size() > 0) // No need to seed a response if the log entries was empty
            rpc_->send_message(response_message);

        persist_state();
    }

    // Leader receives an Append Entries response
    void Server::handle_append_entries_response(const message::Message& message)
    {
        // Outdated term
        if (message.term() > current_term_)
        {
            become_follower(message.term());
            return;
        }

        append_entry::AppendEntriesResponse response;
        message.payload().UnpackTo(&response);

        // Check if the server is the leader
        if (state_ == ServerState::LEADER && current_term_ == message.term())
        {
            // Retrieve index for the server
            index_t server_index = server_indexes_dic_[message.source_id()];

            if (response.success())
            {
                index_t next_index = next_index_.at(server_index);
                next_index_.at(server_index) = next_index + response.nb_log_entries();
                match_index_.at(server_index) = std::make_optional(next_index_.at(server_index) - 1);

                // Used to log if new log entries have to be committed
                std::optional<index_t> last_commit_index = commit_index_;

                index_t begin = commit_index_ ? commit_index_.value() + 1 : 0;

                for (index_t i = begin; i < log_entries_.size(); ++i)
                {
                    if (log_entries_.at(i).term() == current_term_)
                    {
                        // Check how many servers want to commit the new log_entries entries
                        uint32 match_count = 1;
                        for (const auto& id: server_ids_)
                        {
                            if (id != id_)
                            {
                                index_t idx = server_indexes_dic_[id];
                                auto match_index = match_index_.at(idx);

                                if (match_index && match_index.value() >= i)
                                    ++match_count;
                            }
                        }

                        // If we obtain the majority, we commit the new log entries
                        if (match_count >= (server_ids_.size() / 2) + 1)
                            commit_index_ = std::make_optional(i);
                    }
                }

                if (
                    (!last_commit_index && commit_index_) ||
                    (commit_index_ && commit_index_.value() != last_commit_index.value())
                )
                {
                    #ifdef DEBUG
                    std::cout << "Leader commit index changed to " << commit_index_.value() << std::endl;
                    #endif

                    //leader_send_heartbeats();
                }
            }
            else
                next_index_.at(server_index) -= 1;
        }
    }

    void Server::handle_command_entry_request(const message::Message& message)
    {
        #ifdef DEBUG
        std::cout << "Server " << id_ << " received a command from node " << message.source_id() << "!" << std::endl;
        #endif

        if (state_ == ServerState::LEADER)
        {
            command_entry::CommandEntryRequest request;
            message.payload().UnpackTo(&request);

            // Create my new log entry
            log_entry::LogEntry new_entry;
            new_entry.set_client_id(message.source_id());
            new_entry.set_leader_id(message.dest_id());
            new_entry.set_index(log_entries_.size());
            new_entry.set_command(request.command());
            new_entry.set_term(current_term_);

            log_entries_.push_back(new_entry);
            log_entries_to_commit_.emplace(new_entry);

            persist_state();

            leader_send_heartbeats();

            #ifdef DEBUG
            std::cout << "Leader log entries:" << std::endl;
            for (auto entry = log_entries_.begin(); entry != log_entries_.end(); ++entry)
                std::cout << "- Log " << entry->index() << " with command '" << entry->command() << "'" << std::endl;
            #endif
        }
    }

    void Server::handle_search_leader_request(const message::Message& message)
    {
        if (state_ == ServerState::LEADER)
        {
            search_leader::SearchLeaderResponse response;
            response.set_leader_id(id_);

            message::Message response_message;
            response_message.set_source_id(id_);
            response_message.set_dest_id(message.source_id());
            response_message.set_type(message::MessageType::SEARCH_LEADER_RESPONSE);
            response_message.mutable_payload()->PackFrom(response);
            response_message.set_term(current_term_);

            rpc_->send_message(response_message);
        }
    }

    // Handle message depending the message type
    void Server::handle_message(const message::Message& message)
    {
        switch (message.type())
        {
            case message::MessageType::VOTE_REQUEST:
                handle_vote_request(message);
                break;
            case message::MessageType::VOTE_RESPONSE:
                handle_vote_response(message);
                break;
            case message::MessageType::APPEND_ENTRIES_REQUEST:
                handle_append_entries_request(message);
                break;
            case message::MessageType::APPEND_ENTRIES_RESPONSE:
                handle_append_entries_response(message);
                break;
            case message::MessageType::COMMAND_ENTRY_REQUEST:
                handle_command_entry_request(message);
                break;
            case message::MessageType::SEARCH_LEADER_REQUEST:
                handle_search_leader_request(message);
                break;
            default:
                break;
        }
    }

    // Listen to messages from other servers and clients
    void Server::receive_all_messages()
    {
        for (const auto& id: node_ids_)
        {
            if (id == id_) // Ignore the current server messages
                continue;

            while (running_)
            {
                std::optional<message::Message> message = rpc_->receive_message(id);

                if (message)
                    messages_.emplace(message.value());
                else
                    break;
            }
        }
    }

    void Server::check_new_commit_to_apply()
    {
        while (
            (commit_index_ && !last_applied_commit_index_) ||
            (commit_index_ && last_applied_commit_index_ && commit_index_.value() > last_applied_commit_index_.value())
        )
        {
            if (!last_applied_commit_index_)
                last_applied_commit_index_ = std::make_optional(0);
            else
                last_applied_commit_index_ = std::make_optional(last_applied_commit_index_.value() + 1);

            if (state_ == ServerState::LEADER && !log_entries_to_commit_.empty())
            {
                log_entry::LogEntry entry = log_entries_to_commit_.front();

                // Send command entry response
                command_entry::CommandEntryResponse response;
                response.set_command_committed(state_ == ServerState::LEADER && entry.leader_id() == id_);

                message::Message response_message;
                response_message.set_source_id(id_);
                response_message.set_dest_id(entry.client_id());
                response_message.set_type(message::MessageType::COMMAND_ENTRY_RESPONSE);
                response_message.mutable_payload()->PackFrom(response);

                rpc_->send_message(response_message);

                log_entries_to_commit_.pop();

                std::cout << "Log committed by leader with index " << entry.index() << std::endl;
            }
        }
    }

    // Listen to messages from the controller
    void Server::receive_controller_messages()
    {
        while (running_)
        {
            std::optional<message::Message> message = rpc_->receive_message(controller_id_);

            if (message)
                messages_controller_.emplace(message.value());
            else
                break;
        }
    }

    void Server::handle_controller_messages()
    {
        if (!messages_controller_.empty())
        {
            message::Message message = messages_controller_.front();
            handle_controller_message(message);
            messages_controller_.pop();
        }
    }

    void Server::handle_crash_request()
    {
        if (state_ != ServerState::DEAD)
            crash();
        else
        {
            #ifdef DEBUG
            std::cout << "Server " << id_ << " already dead!" << std::endl;
            #endif
        }
    }

    void Server::handle_start_request()
    {
        if (state_ == ServerState::DEAD)
            start();
        else
        {
            #ifdef DEBUG
            std::cout << "Server " << id_ << " already started!" << std::endl;
            #endif
        }
    }

    void Server::handle_election_timeout_request(const message::Message& message)
    {
        election_timeout::ElectionTimeoutRequest request;
        message.payload().UnpackTo(&request);

        if (state_ == ServerState::DEAD)
        {
            election_timeout_ = request.timeout();

            #ifdef DEBUG
            std::cout << "Server " << id_ << " has an election timeout of " << election_timeout_ << std::endl;
            #endif
        }
        else
        {
            #ifdef DEBUG
            std::cout << "Server " << id_ << " already started!" << std::endl;
            #endif
        }
    }

    void Server::handle_speed_request(const message::Message& message)
    {
        speed::SpeedRequest request;
        message.payload().UnpackTo(&request);

        if (request.speed() != speed::Speed::UNKNOWN)
            speed_ = request.speed();
    }

    void Server::handle_controller_message(const message::Message& message)
    {
        switch (message.type())
        {
            case message::MessageType::CRASH_REQUEST:
                handle_crash_request();
                break;
            case message::MessageType::START_REQUEST:
                handle_start_request();
                break;
            case message::MessageType::COMMAND_ENTRY_REQUEST:
                handle_command_entry_request(message);
                break;
            case message::MessageType::ELECTION_TIMEOUT_REQUEST:
                handle_election_timeout_request(message);
                break;
            case message::MessageType::SPEED_REQUEST:
                handle_speed_request(message);
                break;
            case message::MessageType::EXIT:
                running_ = false;
                break;
            default:
                break;
        }
    }
}
