#pragma once

#include "proto/message.pb.h"
#include "proto/command_entry.pb.h"
#include "proto/persistent_state.pb.h"

namespace utils
{
    std::string serialize_message(const message::Message& message);
    std::optional<message::Message> deserialize_message(const std::string& str);
}
