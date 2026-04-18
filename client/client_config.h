#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "../common/protocol.h"

constexpr int MIN_PORT = 1;
constexpr int MAX_PORT = 65535;
constexpr int MIN_TIMEOUT = 1;
constexpr int MAX_TIMEOUT = 99;

struct ClientConfig {
    std::string address;
    int port;
    buffer_t message;
    int timeout;

    void validate() const;
};

buffer_t parse_message(const std::string &s);

#endif
