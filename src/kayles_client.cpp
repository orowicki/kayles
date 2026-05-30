#include <exception>
#include <iostream>
#include <unistd.h>

#include "client.h"
#include "client_args.h"
#include "client_config.h"

using std::cerr;
using std::exception;

int main(int argc, char *argv[])
{
    try {
        ClientConfig cfg = configure_from_args(argc, argv);
        Client client(cfg);
        client.run();

    } catch (exception &e) {
        cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
