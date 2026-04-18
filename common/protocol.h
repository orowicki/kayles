#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <vector>

using buffer_t = std::vector<uint8_t>;
using msg_type_t = uint8_t;
using player_id_t = uint32_t;
using game_id_t = uint32_t;
using game_status_t = uint8_t;
using idx_t = uint8_t;

constexpr size_t BYTE_SIZE = 8;

namespace client
{
namespace field
{
struct MsgType {
    static constexpr size_t OFFSET = 0;
    static constexpr size_t INDEX = 0;
};
struct PlayerID {
    static constexpr size_t OFFSET = 1;
    static constexpr size_t INDEX = 1;
};
struct GameID {
    static constexpr size_t OFFSET = 5;
    static constexpr size_t INDEX = 2;
};
struct Pawn {
    static constexpr size_t OFFSET = 9;
    static constexpr size_t INDEX = 3;
};
} // namespace field

namespace msg
{
struct Join {
    static constexpr msg_type_t TYPE = 0;
    static constexpr size_t SIZE = 5;
    static constexpr size_t FIELDS = 2;
};

struct Move {
    static constexpr msg_type_t TYPE_1 = 1;
    static constexpr msg_type_t TYPE_2 = 2;
    static constexpr size_t SIZE = 10;
    static constexpr size_t FIELDS = 4;
};

struct KeepAlive {
    static constexpr msg_type_t TYPE = 3;
    static constexpr size_t SIZE = 9;
    static constexpr size_t FIELDS = 3;
};

struct GiveUp {
    static constexpr msg_type_t TYPE = 4;
    static constexpr size_t SIZE = 9;
    static constexpr size_t FIELDS = 3;
};
} // namespace msg
} // namespace client

namespace server
{
namespace field
{
struct GameID {
    static constexpr size_t OFFSET = 0;
};
struct PlayerAID {
    static constexpr size_t OFFSET = 4;
};
struct PlayerBID {
    static constexpr size_t OFFSET = 8;
};
struct Status {
    static constexpr size_t OFFSET = 12;
};
struct ErrorIndex {
    static constexpr size_t OFFSET = 13;
};
struct MaxPawn {
    static constexpr size_t OFFSET = 13;
};
struct Board {
    static constexpr size_t OFFSET = 14;
};
} // namespace field

namespace msg
{
struct WrongMsg {
    static constexpr size_t SIZE = 14;
    static constexpr size_t MAX_ECHO_SIZE = 12;
};

struct GameState {
    static constexpr size_t MIN_SIZE = 15;
};
} // namespace msg
} // namespace server

enum class GameStatus : game_status_t {
    WAITING_FOR_OPPONENT = 0,
    TURN_A = 1,
    TURN_B = 2,
    WIN_A = 3,
    WIN_B = 4,
    WRONG_MSG = 255
};

struct Message {
    Message(const buffer_t &b, const sockaddr_in &a, socklen_t l) noexcept
        : buffer(b), client_addr(a), addr_len(l)
    {
    }
    const buffer_t &buffer;
    const sockaddr_in &client_addr;
    socklen_t addr_len;
};

inline uint8_t read_uint8(const buffer_t &buffer, size_t offset)
{
    return buffer[offset];
}

inline uint32_t read_uint32(const buffer_t &buffer, size_t offset)
{
    uint32_t value;
    std::memcpy(&value, buffer.data() + offset, sizeof(uint32_t));
    return ntohl(value);
}

inline void write_uint8(buffer_t &buffer, uint8_t value)
{
    buffer.push_back(value);
}

inline void write_uint32(buffer_t &buffer, uint32_t value)
{
    uint32_t net_value = htonl(value);
    const uint8_t *byte_ptr = reinterpret_cast<const uint8_t *>(&net_value);
    buffer.insert(buffer.end(), byte_ptr, byte_ptr + sizeof(uint32_t));
}

#endif // PROTOCOL_H
