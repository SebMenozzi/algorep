#include "mpi_process.hh"

namespace mpi
{
    int handle_mpi_process(int argc, char **argv, int nb_servers, int nb_clients)
    {
        int rank, size;
        MPI_Init(&argc, &argv);

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        int nb_nodes = nb_servers + nb_clients;

        if (size != nb_nodes + 1)
        {
            std::cerr << "Usage: mpirun -np [NB_PROCESSES = CONTROLLER + NB_SERVERS + NB_CLIENTS]" << std::endl;
            return EXIT_FAILURE;
        }

        // Implementation of RPC using OpenMPI
        auto rpc = mpi::RPC();

        // Rank table
        // Controller => 0
        // Servers => 1 to nb_servers
        // Clients => nb_servers + 1 to nb_nodes

        // All the server ids
        std::vector<raft::node_id_t> server_ids(nb_servers);
        for (int i = 0; i < nb_servers; ++i) { server_ids[i] = i + 1; }

        // All the node ids (clients + servers)
        std::vector<raft::node_id_t> node_ids(nb_nodes);
        for (int i = 0; i < nb_nodes; ++i) { node_ids[i] = i + 1; }

        if (rank == 0)
        {
            auto controller = raft::Controller(rank, server_ids, node_ids);
            controller.set_rpc(&rpc);
            controller.run();
        }
        else
        {
            // Run Server
            if (rank <= nb_servers)
            {
                auto server = raft::Server(rank, 0, server_ids, node_ids);
                server.set_rpc(&rpc);
                server.run();
            }
            // Run Client
            else
            {
                auto client = raft::Client(rank, 0, server_ids);
                client.set_rpc(&rpc);
                client.run();
            }
        }

        MPI_Finalize();

        return EXIT_SUCCESS;
    }
}
