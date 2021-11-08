#include "raft_client.hh"

namespace raft
{
    // MARK - Public

    Client::Client(
        node_id_t id,
        node_id_t controller_id,
        const std::vector<node_id_t> server_ids
    ):
        id_(id),
        controller_id_(controller_id),
        server_ids_(server_ids),
        state_(ClientState::DEAD),
        timeout_(50),
        leader_clock_(),
        command_clock_(),
        leader_id_(std::nullopt),
        commands_to_send_(),
        next_command_sent_(true),
        running_(true)
    {}

    void Client::run()
    {
        #ifdef DEBUG
        std::cout << "Client " << id_ << " running..." << std::endl;
        #endif

        while (running_)
        {
            receive_controller_messages();

            if (state_ != ClientState::DEAD)
            {
                receive_servers_messages();

                if (leader_id_ == std::nullopt)
                    search_leader();
                else if (next_command_sent_)
                    send_next_command();
                else if (!next_command_sent_)
                    check_command_timeout();
            }
        }

        #ifdef DEBUG
        std::cout << "Client " << id_ << " is stopping..." << std::endl;
        #endif

        sleep(1);
    }

    // MARK - Private

    void Client::start()
    {
        #ifdef DEBUG
        std::cout << "Client " << id_ << " started!" << std::endl;
        #endif

        state_ = ClientState::ALIVE;
    }

    void Client::crash()
    {
        #ifdef DEBUG
        std::cerr << "Client " << id_ << " crashed!" << std::endl;
        #endif

        state_ = ClientState::DEAD;
        reset_leader();

        // Clear the commands queue
        while(!commands_to_send_.empty())
            commands_to_send_.pop();

        // Ready to send new commands
        next_command_sent_ = true;
    }

    // Listen to messages from servers
    void Client::receive_servers_messages()
    {
        for (const auto& id: server_ids_)
        {
            while (running_)
            {
                std::optional<message::Message> message = rpc_->receive_message(id);

                if (message.has_value())
                    handle_server_message(message.value());
                else
                    break;
            }
        }
    }

    void Client::handle_search_leader_response(const message::Message& message)
    {
        search_leader::SearchLeaderResponse response;
        message.payload().UnpackTo(&response);

        leader_id_ = std::make_optional(response.leader_id());

        #ifdef DEBUG
        std::cout << "Client " << id_ << " found leader " << leader_id_.value() << std::endl;
        #endif
    }

    void Client::handle_command_entry_response(const message::Message& message)
    {
        command_entry::CommandEntryResponse response;
        message.payload().UnpackTo(&response);

        if (response.command_committed())
            commands_to_send_.pop();
        else
            reset_leader();

        next_command_sent_ = true;
    }

    // Handle server message depending the message type
    void Client::handle_server_message(const message::Message& message)
    {
        switch (message.type())
        {
            case message::MessageType::SEARCH_LEADER_RESPONSE:
                handle_search_leader_response(message);
                break;
            case message::MessageType::COMMAND_ENTRY_RESPONSE:
                handle_command_entry_response(message);
            default:
                break;
        }
    }

    // Listen to messages from the controller
    void Client::receive_controller_messages()
    {
        while (running_)
        {
            std::optional<message::Message> message = rpc_->receive_message(controller_id_);

            if (message.has_value())
                handle_controller_message(message.value());
            else
                break;
        }
    }

    void Client::handle_crash_request()
    {
        if (state_ == ClientState::ALIVE)
            crash();
        else
        {
            #ifdef DEBUG
            std::cout << "Client " << id_ << " already dead!" << std::endl;
            #endif
        }
    }

    void Client::handle_start_request()
    {
        if (state_ != ClientState::ALIVE)
            start();
        else
        {
            #ifdef DEBUG
            std::cout << "Client " << id_ << " already started!" << std::endl;
            #endif
        }
    }

    void Client::handle_command_entry_request(const message::Message& message)
    {
        #ifdef DEBUG
        std::cout << "Client " << id_ << " received a command from controller!" << std::endl;
        #endif

        if (state_ == ClientState::ALIVE)
        {
            command_entry::CommandEntryRequest request;
            message.payload().UnpackTo(&request);

            commands_to_send_.emplace(request.command());
        }
    }

    void Client::handle_controller_message(const message::Message& message)
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
            case message::MessageType::EXIT:
                running_ = false;
                break;
            default:
                break;
        }
    }

    void Client::search_leader()
    {
        if (leader_clock_.get_time() >= timeout_)
        {
            leader_clock_.reset();

            // Send search leader request to all servers
            message::Message message;
            message.set_source_id(id_);
            message.set_type(message::MessageType::SEARCH_LEADER_REQUEST);

            // Ask every servers to know who is the leader
            for (const auto& id: server_ids_)
            {
                message.set_dest_id(id);
                rpc_->send_message(message);
            }
        }
    }

    void Client::reset_leader()
    {
        if (leader_id_ != std::nullopt)
        {
            leader_id_ = std::nullopt;
            leader_clock_.reset();
        }
    }

    void Client::send_next_command()
    {
        if (!commands_to_send_.empty() && leader_id_.has_value())
        {
            std::string command = commands_to_send_.front();

            // Send the command to the leader

            command_entry::CommandEntryRequest request;
            request.set_command(command);

            message::Message message;
            message.set_source_id(id_);
            message.set_dest_id(leader_id_.value());
            message.set_type(message::MessageType::COMMAND_ENTRY_REQUEST);
            message.mutable_payload()->PackFrom(request);
            rpc_->send_message(message);

            next_command_sent_ = false;
            command_clock_.reset();
        }
    }

    void Client::check_command_timeout()
    {
        if (!commands_to_send_.empty() && leader_clock_.get_time() >= timeout_)
        {
            next_command_sent_ = true;
            reset_leader();
        }
    }
}
