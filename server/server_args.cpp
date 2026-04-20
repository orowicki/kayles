#include <stdexcept>
#include <string>
#include <unistd.h>

#include "../common/utils.h"
#include "server_args.h"
#include "server_config.h"

using std::invalid_argument;
using std::string;

namespace
{

ServerConfig parse_args(int argc, char *argv[])
{
    string pawn_string, address;
    int port = -1, timeout = -1;

    int opt;
    opterr = 0;

    while ((opt = getopt(argc, argv, "r:a:p:t:")) != -1) {
        switch (opt) {
            case 'r':
                pawn_string = optarg;
                break;

            case 'a':
                address = optarg;
                break;

            case 'p':
                port = parse_int(optarg, "Invalid port number!");
                break;

            case 't':
                timeout = parse_int(optarg, "Invalid timeout length!");
                break;

            case '?':
                throw invalid_argument("Unknown argument or missing value!");
        }
    }

    if (pawn_string.empty())
        throw invalid_argument("Missing -r (pawn row)");
    if (address.empty())
        throw invalid_argument("Missing -a (address)");
    if (port == -1)
        throw invalid_argument("Missing -p (port)");
    if (timeout == -1)
        throw invalid_argument("Missing -t (timeout)");

    if (optind < argc)
        throw invalid_argument("Unexpected extra positional arguments!");

    ServerConfig cfg{
        .pawn_row = parse_pawn_row(pawn_string),
        .address = address,
        .port = port,
        .timeout = timeout,
        .max_pawn = static_cast<idx_t>(pawn_string.size() - 1),
    };

    return cfg;
}

} // namespace

ServerConfig configure_from_args(int argc, char *argv[])
{
    ServerConfig cfg = parse_args(argc, argv);

    cfg.validate();

    return cfg;
}
