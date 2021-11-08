#include "mpi_rpc.hh"

namespace mpi
{
    void RPC::send_message(const message::Message& message)
    {
        std::string serialized_message = utils::serialize_message(message);

        MPI_Request request;
        MPI_Isend(
            serialized_message.c_str(),
            serialized_message.size(),
            MPI_CHAR,
            message.dest_id(),
            0,
            MPI_COMM_WORLD,
            &request
        );

        MPI_Request_free(&request);
    }

    std::optional<message::Message> RPC::receive_message(raft::node_id_t id)
    {
        MPI_Status mpi_status;
        int flag;
        MPI_Iprobe(id, 0, MPI_COMM_WORLD, &flag, &mpi_status);

        if (!flag)
            return std::nullopt;

        int buffer_size = 0;
        MPI_Get_count(&mpi_status, MPI_CHAR, &buffer_size);

        std::vector<char> buffer(buffer_size);

        MPI_Request request;
        MPI_Irecv(buffer.data(), buffer_size, MPI_CHAR, id, 0, MPI_COMM_WORLD, &request);
        MPI_Request_free(&request);

        std::string serialized_message(buffer.begin(), buffer.end());

        return utils::deserialize_message(serialized_message);
    }
}
