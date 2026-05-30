#include <stdexcept>
#include <string>
#include <unistd.h>

#include "client_args.h"
#include "client_config.h"
#include "utils.h"

using std::invalid_argument;
using std::string;

namespace
{

ClientConfig parse_args(int argc, char *argv[])
{
    string message, address;
    int port = -1, timeout = -1;

    int opt;
    opterr = 0;

    while ((opt = getopt(argc, argv, "a:p:m:t:")) != -1) {
        switch (opt) {
            case 'a':
                address = optarg;
                break;

            case 'p':
                port = parse_int(optarg, "Invalid port number!");
                break;

            case 'm':
                message = optarg;
                break;

            case 't':
                timeout = parse_int(optarg, "Invalid timeout length!");
                break;

            case '?':
                throw invalid_argument("Unknown argument or missing value!");
        }
    }

    if (message.empty())
        throw invalid_argument("Missing or invalid -m (message)");
    if (address.empty())
        throw invalid_argument("Missing or invalid -a (address)");
    if (port == -1)
        throw invalid_argument("Missing or invalid -p (port)");
    if (timeout == -1)
        throw invalid_argument("Missing or invalid -t (timeout)");

    if (optind < argc)
        throw invalid_argument("Unexpected extra positional arguments!");

    ClientConfig cfg{
        .address = address,
        .port = port,
        .message = parse_message(message),
        .timeout = timeout,
    };

    return cfg;
}

} /* namespace */

ClientConfig configure_from_args(int argc, char *argv[])
{
    ClientConfig cfg = parse_args(argc, argv);

    cfg.validate();

    return cfg;
}
