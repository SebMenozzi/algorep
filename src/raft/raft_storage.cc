#include "raft_storage.hh"

namespace raft
{
    Storage::Storage(node_id_t id):
        path_("logs/server_" + std::to_string(id) + ".data")
    {
        boost::filesystem::create_directory("logs");
    }

    void Storage::save(const persistent_state::PersistentState& state)
    {
        std::ofstream file(path_, std::ios_base::out | std::ios_base::binary);
        state.SerializeToOstream(&file);
        file.close();
    }

    persistent_state::PersistentState Storage::get()
    {
        std::ifstream file(path_, std::ios_base::in | std::ios_base::binary);
        persistent_state::PersistentState state;
        state.ParseFromIstream(&file);
        file.close();
        return state;
    }

    bool Storage::has_data()
    {
        return boost::filesystem::exists(path_) && boost::filesystem::file_size(path_) != 0;
    }
}
