#pragma once

// #include <google/protobuf/message.h> // google::protobuf::MessageLite
#include "proto/persistent_state.pb.h"

namespace storage
{
    class Storage
    {
        public:
            virtual ~Storage() {}

            // TODO: @sebmenozzi I would like to use google::protobuf::MessageLite instead
            // but I can't use a abstract class in a interface, should probably use generics!
            virtual void save(const persistent_state::PersistentState& state) = 0;
            virtual persistent_state::PersistentState get() = 0;
            virtual bool has_data() = 0;
    };
}
