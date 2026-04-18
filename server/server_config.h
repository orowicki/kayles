#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <cstdint>
#include <string>

#include "../common/protocol.h"

constexpr int MIN_PORT = 0;
constexpr int MAX_PORT = 65535;
constexpr size_t MIN_PAWNS = 1;
constexpr size_t MAX_PAWNS = 256;
constexpr int MIN_TIMEOUT = 1;
constexpr int MAX_TIMEOUT = 99;

struct ServerConfig {
    buffer_t pawn_row;
    std::string address;
    int port;
    int timeout;
    idx_t max_pawn;

    void validate() const;
};

buffer_t parse_pawn_row(const std::string &s);

#endif
