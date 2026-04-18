#include <iostream>
#include <string>
#include <unistd.h>

#include "args.h"
#include "client.h"
#include "client_config.h"

using std::cerr;
using std::exception;
using std::string;

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
