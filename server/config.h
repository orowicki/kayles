#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

constexpr int MIN_PORT = 0;
constexpr int MAX_PORT = std::numeric_limits<uint16_t>::max();
constexpr int MIN_PAWNS = 1;
constexpr int MAX_PAWNS = std::numeric_limits<uint8_t>::max();
constexpr int MIN_TIMEOUT = 1;
constexpr int MAX_TIMEOUT = 99;

struct Config {
    std::vector<uint8_t> pawn_row;
    std::string address;
    int port;
    int timeout;

    void validate() const;
};

std::vector<uint8_t> parse_pawn_row(const std::string &);

#endif
