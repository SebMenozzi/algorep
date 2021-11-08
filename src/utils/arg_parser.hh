#pragma once

#include <iostream>
#include <boost/program_options.hpp>

#include "mpi_process.hh"

namespace po = boost::program_options;

namespace utils
{
    int handle_arg_parser(int argc, char** argv);
}
