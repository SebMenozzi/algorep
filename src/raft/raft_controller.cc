#include "raft_controller.hh"

namespace raft
{
    Controller::Controller(
        node_id_t id, 
        const std::vector<node_id_t> server_ids,
        const std::vector<node_id_t> node_ids
    ):
        id_(id),
        server_ids_(server_ids),
        node_ids_(node_ids),
        clock_()
    {}

    void Controller::send_command_request(node_id_t id, const std::string& str)
    {
        // Send the command to the leader
        command_entry::CommandEntryRequest request;
        request.set_command(str);

        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::COMMAND_ENTRY_REQUEST);
        message.mutable_payload()->PackFrom(request);
        message.set_dest_id(id);
        rpc_->send_message(message);
    }

    void Controller::send_crash_request(node_id_t id)
    {
        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::CRASH_REQUEST);
        message.set_dest_id(id);
        rpc_->send_message(message);
    }

    void Controller::send_start_request(node_id_t id)
    {
        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::START_REQUEST);
        message.set_dest_id(id);
        rpc_->send_message(message);
    }

    void Controller::send_exit_request(node_id_t id)
    {
        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::EXIT);
        message.set_dest_id(id);
        rpc_->send_message(message);
    }

    void Controller::send_election_timeout_request(node_id_t id, time_t timeout)
    {
        election_timeout::ElectionTimeoutRequest request;
        request.set_timeout(timeout);

        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::ELECTION_TIMEOUT_REQUEST);
        message.set_dest_id(id);
        message.mutable_payload()->PackFrom(request);
        rpc_->send_message(message);
    }

    void Controller::send_speed_request(node_id_t id, speed::Speed speed)
    {
        speed::SpeedRequest request;
        request.set_speed(speed);

        message::Message message;
        message.set_source_id(id_);
        message.set_type(message::MessageType::SPEED_REQUEST);
        message.set_dest_id(id);
        message.mutable_payload()->PackFrom(request);
        rpc_->send_message(message);
    }

    speed::Speed Controller::string_to_speed(const std::string& str)
    {
        speed::Speed speed = speed::Speed::UNKNOWN;

        if (str == "NONE")
            speed = speed::Speed::NONE;
        else if (str == "LOW")
            speed = speed::Speed::LOW;
        else if (str == "MEDIUM")
            speed = speed::Speed::MEDIUM;
        else if (str == "HIGH")
            speed = speed::Speed::HIGH;

        return speed;
    }

    void Controller::run()
    {
        #ifdef DEBUG
        std::cout << "Controller is accepting inputs..." << std::endl;
        #endif

        for (std::string line; std::getline(std::cin, line); )
        {
            std::vector<std::string> result;
            boost::split(result, line, boost::is_any_of(" "));

            try
            {
                if (result.size() > 0)
                {
                    std::string command = result[0];

                    if (result.size() == 1)
                    {
                        if (command == "START_SERVERS")
                        {
                            for (const auto& id: server_ids_)
                                send_start_request(id);

                            #ifdef DEBUG
                            std::cout << "Sending a start request to all servers..." << std::endl;
                            #endif

                            continue;
                        }

                        if (command=="EXIT")
                        {
                            for (const auto& id: node_ids_)
                                send_exit_request(id);

                            #ifdef DEBUG
                            std::cout << "Terminating the program" << std::endl;
                            #endif

                            break;
                        }
                    }
                    else if (result.size() >= 2)
                    {
                        node_id_t node_id = std::stoi(result[1]);

                        if (command == "CRASH")
                        {
                            send_crash_request(node_id);

                            #ifdef DEBUG
                            std::cout << "Sending a crash request to node " << node_id << "..." << std::endl;
                            #endif

                            continue;
                        }
                        else if (command == "START" || command == "RECOVER")
                        {
                            send_start_request(node_id);

                            #ifdef DEBUG
                            std::cout << "Sending a start request to node " << node_id << "..." << std::endl;
                            #endif

                            continue;
                        }

                        if (result.size() == 3)
                        {
                            if (command == "SEND_COMMAND")
                            {
                                std::string str = result[2];

                                send_command_request(node_id, str);

                                #ifdef DEBUG
                                std::cout << "Sending a command request to node " << node_id << "..." << std::endl;
                                #endif

                                continue;
                            }
                            else if (command == "SET_ELECTION_TIMEOUT")
                            {
                                uint32 timeout = std::stoi(result[2]);

                                send_election_timeout_request(node_id, timeout);

                                continue;
                            }
                            if (command == "SPEED")
                            {
                                std::string str = result[2];

                                speed::Speed speed = string_to_speed(str);

                                if (speed == speed::Speed::UNKNOWN)
                                {
                                    #ifdef DEBUG
                                    std::cout << "Unknown speed type '" << str << "'" << std::endl;
                                    #endif
                                    continue;
                                }

                                send_speed_request(node_id, speed);

                                #ifdef DEBUG
                                std::cout << "Sending a speed request '" << str << "' to node " << node_id << "..." << std::endl;
                                #endif

                                continue;
                            }
                        }
                    }
                }
            }
            catch(...) {}

            #ifdef DEBUG
            std::cout << "Unrecognized command '" << line << "'" << std::endl;
            #endif
        }

        #ifdef DEBUG
        std::cout << "Controller is stopping..." << std::endl;
        #endif

        sleep(1);
    }
}
