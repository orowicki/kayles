#ifndef UTILS_H
#define UTILS_H

#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "protocol.h"

inline std::pair<size_t, size_t> find_bit_idxs(size_t idx) noexcept
{
    return { idx / BYTE_SIZE, (BYTE_SIZE - 1) - (idx % BYTE_SIZE) };
}

inline int parse_int(const std::string &s, const std::string &error_msg)
{
    int res;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), res);

    if (ec != std::errc{} || p != s.data() + s.size())
        throw std::invalid_argument(error_msg);

    return res;
}

#endif
