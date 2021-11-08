#pragma once

#include <iostream>
#include <fstream> // std::ifstream std::fstream
#include <boost/filesystem.hpp> // boost::filesystem::create_directory

#include "storage.hh"
#include "raft_types.hh"

#include "proto/persistent_state.pb.h"

namespace raft
{
    class Storage: public storage::Storage
    {
        public:
            Storage(node_id_t id);
            // Overriden methods
            void save(const persistent_state::PersistentState& state) override;
            persistent_state::PersistentState get() override;
            bool has_data() override;
        private:
            std::string path_;
    };
}
