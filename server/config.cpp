#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{

constexpr int BYTE = 8;

void validate_address(const string &s) {}

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
    if (s.size() < MIN_PAWNS || s.size() > MAX_PAWNS || s.front() != '1' ||
        s.back() != '1')
        throw invalid_argument("Invalid pawn_row!");

    for (auto c : s)
        if (c != '0' && c != '1')
            throw invalid_argument("Invalid pawn_row!");
}

} // namespace

void Config::validate() const
{
    validate_address(address);
    validate_port(port);
    validate_timeout(timeout);
}

vector<uint8_t> parse_pawn_row(const string &s)
{
    validate_pawn_string(s);
    size_t n = s.size();
    vector<uint8_t> pawn_row((n + (BYTE - 1)) / BYTE, 0);

    for (size_t i = 0; i < n; ++i) {
        char c = s[i];

        if (c == '1') {
            size_t byte_idx = i / BYTE;
            size_t bit_idx = i % BYTE;
            pawn_row[byte_idx] |= (1 << bit_idx);
        }
    }

    return pawn_row;
}
