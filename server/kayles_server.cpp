#include <iostream>
#include <string>
#include <unistd.h>

#include "args.h"
#include "server.h"
#include "server_config.h"

using std::cerr;
using std::exception;
using std::string;

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
