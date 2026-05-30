#include <exception>
#include <iostream>
#include <unistd.h>

#include "server.h"
#include "server_args.h"
#include "server_config.h"

using std::cerr;
using std::exception;

int main(int argc, char *argv[])
{
    try {
        ServerConfig cfg = configure_from_args(argc, argv);
        Server server(cfg);
        server.run();

    } catch (exception &e) {
        cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
