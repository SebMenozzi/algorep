#pragma once

#include <iostream>
#include <string>
#include <mpi.h>

#include "types.hh"
#include "raft_types.hh"
#include "mpi_rpc.hh"
#include "raft_controller.hh"
#include "raft_server.hh"
#include "raft_client.hh"

namespace mpi
{
    int handle_mpi_process(int argc, char **argv, int nb_servers, int nb_clients);
}
