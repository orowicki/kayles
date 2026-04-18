#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "../common/protocol.h"
#include "../common/utils.h"
#include "server_config.h"

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{

void validate_address(const string &s)
{
    if (s.empty())
        throw invalid_argument("Address not provided!");

    struct addrinfo hints{};
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(s.c_str(), nullptr, &hints, &res) != 0)
        throw invalid_argument("Invalid IPv4 address or domain name: " + s);

    freeaddrinfo(res);
}

void validate_port(int port)
{
    if (port < MIN_PORT || port > MAX_PORT)
        throw invalid_argument("Invalid port number!");
}

void validate_timeout(int timeout)
{
    if (timeout < MIN_TIMEOUT || timeout > MAX_TIMEOUT)
        throw invalid_argument("Invalid timeout length!");
}

void validate_pawn_string(const string &s)
{
    if (s.empty() || s.size() < MIN_PAWNS || s.size() > MAX_PAWNS ||
        s.front() != '1' || s.back() != '1')
        throw invalid_argument("Invalid pawn row format!");

    for (auto c : s)
        if (c != '0' && c != '1')
            throw invalid_argument("Invalid characters in pawn row!");
}

} /* namespace */

void ServerConfig::validate() const
{
    validate_address(address);
    validate_port(port);
    validate_timeout(timeout);
}

buffer_t parse_pawn_row(const string &s)
{
    validate_pawn_string(s);
    size_t n = s.size();
    buffer_t pawn_row((n + (BYTE_SIZE - 1)) / BYTE_SIZE, 0);

    for (size_t i = 0; i < n; ++i) {
        if (s[i] == '1') {
            auto [byte_idx, bit_idx] = find_bit_idxs(i);
            pawn_row[byte_idx] |= (1 << bit_idx);
        }
    }

    return pawn_row;
}
