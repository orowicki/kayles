#include <charconv>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

#include "args.h"
#include "config.h"

using std::errc;
using std::from_chars;
using std::string;

namespace
{

int parse_int(const string &s, const string &error_msg)
{
    int res;
    auto [p, ec] = from_chars(s.data(), s.data() + s.size(), res);

    if (ec != errc{})
        throw std::invalid_argument(error_msg);

    return res;
}

Config parse_args(int argc, char *argv[])
{
    string pawn_string, address;
    int port, timeout;

    int opt;

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
                throw std::invalid_argument("Invalid args");
        }
    }

    Config cfg{
        .pawn_row = parse_pawn_row(pawn_string),
        .address = address,
        .port = port,
        .timeout = timeout,
    };

    return cfg;
}

} // namespace

Config configure_from_args(int argc, char *argv[])
{
    Config cfg = parse_args(argc, argv);

    cfg.validate();

    return cfg;
}
