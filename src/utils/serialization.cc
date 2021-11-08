#include "serialization.hh"

namespace utils
{
    std::string serialize_message(const message::Message& message)
    {
        std::string str;
        message.SerializeToString(&str);
        return str;
    }

    std::optional<message::Message> deserialize_message(const std::string& str)
    {
        message::Message message;
        if (!message.ParseFromString(str))
            return std::nullopt;

        return std::make_optional(message);
    }
}
