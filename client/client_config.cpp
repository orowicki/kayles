#include <charconv>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "../common/protocol.h"
#include "../common/utils.h"
#include "client_config.h"

using std::errc;
using std::from_chars;
using std::invalid_argument;
using std::numeric_limits;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

namespace cf = client::field;
namespace cm = client::msg;

namespace
{

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

vector<uint32_t> get_fields(const string &s)
{
    vector<uint32_t> fields;
    stringstream ss(s);
    string token;

    while (getline(ss, token, '/')) {
        if (token.empty())
            throw invalid_argument("Empty field detected in message!");

        uint32_t val;
        auto [p, ec] =
            from_chars(token.data(), token.data() + token.size(), val, 10);

        if (ec != errc{} || p != token.data() + token.size())
            throw invalid_argument("Invalid field: '" + token + "'");

        fields.push_back(val);
    }

    if (fields.empty())
        throw invalid_argument("No fields found in message!");

    return fields;
}

void write_join_fields(const vector<uint32_t> &fields, buffer_t &buf)
{
    if (fields.size() != cm::Join::FIELDS)
        throw invalid_argument("MSG_JOIN requires " +
                               to_string(cm::Join::FIELDS) + " fields!");

    if (fields[cf::PlayerID::INDEX] == 0)
        throw invalid_argument("Player ID cannot be equal 0!");

    write_uint32(buf, fields[cf::PlayerID::INDEX]);
}

void write_move_fields(const vector<uint32_t> &fields, buffer_t &buf)
{
    if (fields.size() != cm::Move::FIELDS)
        throw invalid_argument("MSG_MOVE requires " +
                               to_string(cm::Move::FIELDS) + " fields!");

    if (fields[cf::PlayerID::INDEX] == 0)
        throw invalid_argument("Player ID cannot be equal 0!");

    if (fields[cf::Pawn::INDEX] > numeric_limits<idx_t>::max())
        throw invalid_argument("Pawn index exceeds limit (" +
                               to_string(numeric_limits<idx_t>::max()) + ")!");

    write_uint32(buf, fields[cf::PlayerID::INDEX]);
    write_uint32(buf, fields[cf::GameID::INDEX]);
    write_uint8(buf, fields[cf::Pawn::INDEX]);
}

void write_give_up_fields(const vector<uint32_t> &fields, buffer_t &buf)
{
    if (fields.size() != cm::GiveUp::FIELDS)
        throw invalid_argument("GIVE_UP requires " +
                               to_string(cm::GiveUp::FIELDS) + " fields!");

    if (fields[cf::PlayerID::INDEX] == 0)
        throw invalid_argument("Player ID cannot be equal 0!");

    write_uint32(buf, fields[cf::PlayerID::INDEX]);
    write_uint32(buf, fields[cf::GameID::INDEX]);
}

void write_keep_alive_fields(const vector<uint32_t> &fields, buffer_t &buf)
{
    if (fields.size() != cm::KeepAlive::FIELDS)
        throw invalid_argument("KEEP_ALIVE requires " +
                               to_string(cm::KeepAlive::FIELDS) + " fields!");

    if (fields[cf::PlayerID::INDEX] == 0)
        throw invalid_argument("Player ID cannot be equal 0!");

    write_uint32(buf, fields[cf::PlayerID::INDEX]);
    write_uint32(buf, fields[cf::GameID::INDEX]);
}

void write_fields(const vector<uint32_t> &fields, buffer_t &buf)
{
    uint32_t msg_type = fields[cf::MsgType::INDEX];
    write_uint8(buf, msg_type);

    switch (msg_type) {
        case cm::Join::TYPE:
            write_join_fields(fields, buf);
            break;

        case cm::Move::TYPE_1:
        case cm::Move::TYPE_2:
            write_move_fields(fields, buf);
            break;

        case cm::GiveUp::TYPE:
            write_give_up_fields(fields, buf);
            break;

        case cm::KeepAlive::TYPE:
            write_keep_alive_fields(fields, buf);
            break;

        default:
            throw invalid_argument("Unknown message type: " +
                                   to_string(msg_type));
    }
}

} /* namespace */

void ClientConfig::validate() const
{
    validate_address(address);
    validate_port(port);
    validate_timeout(timeout);
}

buffer_t parse_message(const string &s)
{
    if (s.empty())
        throw invalid_argument("Message string is empty!");

    if (s.front() == '/' || s.back() == '/')
        throw invalid_argument(
            "Message string has extra seperators ('/') on the outside!");

    vector<uint32_t> fields = get_fields(s);
    buffer_t buf;

    write_fields(fields, buf);

    return buf;
}
