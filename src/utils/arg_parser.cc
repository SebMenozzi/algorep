#include "arg_parser.hh"

namespace utils
{
    int handle_arg_parser(int argc, char **argv)
    {
        try
        {
            int nb_servers = 1; // Default number of servers
            int nb_clients = 1; // Default number of clients

            po::options_description desc("Allowed Options");
            desc.add_options()
                ("help, h", "Show Usage")
                ("servers, s", po::value<int>(), "Setup the number of servers")
                ("clients, c", po::value<int>(), "Setup the number of clients")
            ;

            po::variables_map vm;
            po::store(parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            // Help option: --help or --h
            if (vm.count("help"))
            {
                std::cout << desc << "\n";
                exit(EXIT_SUCCESS);
            }

            // Number of servers option: --servers or --s
            if (vm.count("servers"))
            {
                nb_servers = vm["servers"].as<int>();

                if (nb_servers < 0)
                {
                    std::cerr << "Invalid number of servers: " << nb_servers << std::endl;
                    return EXIT_FAILURE;
                }
            }

            // Number of clients option: --clients or --c
            if (vm.count("clients"))
            {
                nb_clients = vm["clients"].as<int>();

                if (nb_clients < 0)
                {
                    std::cerr << "Invalid number of clients: " << nb_clients << std::endl;
                    return EXIT_FAILURE;
                }
            }

            // Handles the MPI process
            return mpi::handle_mpi_process(argc, argv, nb_servers, nb_clients);
        }
        catch (const po::error &e)
        {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
}
