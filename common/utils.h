#ifndef UTILS_H
#define UTILS_H

#include <charconv>
#include <cstddef>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "protocol.h"

/* returns (byte_index, bit_index_from_msb) */
inline std::pair<size_t, size_t> find_bit_idxs(size_t idx) noexcept
{
    return { idx / BYTE_SIZE, (BYTE_SIZE - 1) - (idx % BYTE_SIZE) };
}

inline int parse_int(const std::string &s, const std::string &error_msg)
{
    int res = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), res);

    if (ec != std::errc{} || p != s.data() + s.size())
        throw std::invalid_argument(error_msg);

    return res;
}

inline void validate_address(const std::string &s)
{
    if (s.empty())
        throw std::invalid_argument("Address not provided!");

    struct addrinfo hints{};
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo(s.c_str(), nullptr, &hints, &res);
    if (err != 0)
        throw std::invalid_argument(gai_strerror(err));

    freeaddrinfo(res);
}

#endif /* UTILS_H */
