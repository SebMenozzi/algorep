#include <iostream>
#include <google/protobuf/stubs/common.h>
#include "mpi.h"

#include <boost/mpi/environment.hpp>

#include "arg_parser.hh"

int main(int argc, char** argv)
{
    if (utils::handle_arg_parser(argc, argv) == EXIT_SUCCESS)
    {
        // Delete all global objects allocated by libprotobuf.
        google::protobuf::ShutdownProtobufLibrary();

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
