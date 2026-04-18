#include <algorithm>
#include <arpa/inet.h>
#include <limits>
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>

#include "server.h"

using std::exception;
using std::find_if;
using std::min;
using std::numeric_limits;
using std::runtime_error;
using std::to_underlying;

namespace cf = client::field;
namespace cm = client::msg;

Server::Server(const ServerConfig &config)
    : cfg(config), socket_fd(-1), next_game_id(0)
{
    setup_socket();
}

Server::~Server()
{
    if (socket_fd != -1)
        close(socket_fd);
}

void Server::setup_socket()
{
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cfg.port);

    if (cfg.address == "0.0.0.0") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(cfg.address.c_str(), nullptr, &hints, &res) != 0)
            throw runtime_error("Failed to resolve bind address");

        server_addr.sin_addr.s_addr =
            reinterpret_cast<sockaddr_in *>(res->ai_addr)->sin_addr.s_addr;

        freeaddrinfo(res);
    }

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        throw runtime_error("Failed to create socket");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(socket_fd);
        throw runtime_error("Failed to set SO_RCVTIMEO");
    }

    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&server_addr),
             sizeof(server_addr)) < 0) {
        close(socket_fd);
        throw runtime_error("Failed to bind socket");
    }
}

void Server::run()
{
    buffer_t buffer(BUFFER_SIZE);
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        try {
            ssize_t received = recvfrom(
                socket_fd, buffer.data(), buffer.size(), 0,
                reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);

            if (received >= 0) {
                buffer_t data(buffer.begin(), buffer.begin() + received);
                Message msg(data, client_addr, client_addr_len);
                handle_message(msg);
            }

            cleanup_expired_games();

        } catch (exception &e) {
            // Ignore any exceptions
        }
    }
}

void Server::cleanup_expired_games()
{
    for (auto it = games.begin(); it != games.end();) {
        if (it->second.process_timeouts(cfg.timeout)) {
            it = games.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::send_game_state(const GameState &game,
                             const Message &original_msg) const
{
    buffer_t response;

    write_uint32(response, game.game_id);
    write_uint32(response, game.player_a_id);
    write_uint32(response, game.player_b_id);
    write_uint8(response, to_underlying(game.status));
    write_uint8(response, game.max_pawn);

    response.insert(response.end(), game.pawn_row.begin(), game.pawn_row.end());

    sendto(socket_fd, response.data(), response.size(), 0,
           reinterpret_cast<const sockaddr *>(&original_msg.client_addr),
           original_msg.addr_len);
}

void Server::send_wrong_msg(size_t error_offset,
                            const Message &original_msg) const
{
    buffer_t response;

    size_t copy_len =
        min(original_msg.buffer.size(), server::msg::WrongMsg::MAX_ECHO_SIZE);
    response.insert(response.end(), original_msg.buffer.begin(),
                    original_msg.buffer.begin() + copy_len);

    response.resize(server::msg::WrongMsg::MAX_ECHO_SIZE, 0);

    write_uint8(response, to_underlying(GameStatus::WRONG_MSG));
    write_uint8(response, static_cast<idx_t>(error_offset));

    sendto(socket_fd, response.data(), response.size(), 0,
           reinterpret_cast<const sockaddr *>(&original_msg.client_addr),
           original_msg.addr_len);
}

void Server::handle_message(const Message &msg)
{
    msg_type_t msg_type;
    if (!try_read_msg_type(msg_type, msg))
        return;

    switch (msg_type) {
        case cm::Join::TYPE:
            handle_join(msg);
            break;
        case cm::Move::TYPE_1:
        case cm::Move::TYPE_2:
            handle_move(msg);
            break;
        case cm::KeepAlive::TYPE:
            handle_keep_alive(msg);
            break;
        case cm::GiveUp::TYPE:
            handle_give_up(msg);
            break;
        default:
            send_wrong_msg(cf::MsgType::OFFSET, msg);
    }
}

void Server::handle_join(const Message &msg)
{
    if (!check_message_length(cm::Join::SIZE, msg))
        return;

    player_id_t player_id = read_uint32(msg.buffer, cf::PlayerID::OFFSET);

    if (player_id == 0) {
        send_wrong_msg(cf::PlayerID::OFFSET, msg);
        return;
    }

    auto it = find_if(games.begin(), games.end(), [](const auto &pair) {
        return pair.second.status == GameStatus::WAITING_FOR_OPPONENT;
    });

    if (it != games.end()) {
        it->second.player_b_id = player_id;
        it->second.status = GameStatus::TURN_B;
        it->second.touch(player_id);
        send_game_state(it->second, msg);
    } else {
        if (out_of_ids)
            return;

        out_of_ids = next_game_id == numeric_limits<game_id_t>::max();
        game_id_t game_id = next_game_id++;

        auto [it, inserted] = games.emplace(
            game_id, GameState(game_id, player_id, cfg.max_pawn, cfg.pawn_row));
        send_game_state(it->second, msg);
    }
}

void Server::handle_move(const Message &msg)
{
    GameMap::iterator it;
    if (!try_read_game(it, msg))
        return;

    GameState &game = it->second;

    player_id_t player_id;
    if (!try_read_player_id(player_id, game, msg))
        return;

    if (!check_message_length(cm::Move::SIZE, msg))
        return;

    msg_type_t move_type = read_uint8(msg.buffer, cf::MsgType::OFFSET);
    idx_t pawn_idx = read_uint8(msg.buffer, cf::Pawn::OFFSET);

    if (game.is_their_turn(player_id)) {
        if (move_type == cm::Move::TYPE_1) {
            game.knock_pawns(pawn_idx, 1);
        } else {
            game.knock_pawns(pawn_idx, 2);
        }
    }

    game.touch(player_id);
    send_game_state(game, msg);
}

void Server::handle_keep_alive(const Message &msg)
{
    GameMap::iterator it;
    if (!try_read_game(it, msg))
        return;

    GameState &game = it->second;

    player_id_t player_id;
    if (!try_read_player_id(player_id, game, msg))
        return;

    if (!check_message_length(cm::KeepAlive::SIZE, msg))
        return;

    game.touch(player_id);
    send_game_state(game, msg);
}

void Server::handle_give_up(const Message &msg)
{
    GameMap::iterator it;
    if (!try_read_game(it, msg))
        return;

    GameState &game = it->second;

    player_id_t player_id;
    if (!try_read_player_id(player_id, game, msg))
        return;

    if (!check_message_length(cm::GiveUp::SIZE, msg))
        return;

    if (game.is_their_turn(player_id))
        game.status = game.status == GameStatus::TURN_A ? GameStatus::WIN_B
                                                        : GameStatus::WIN_A;

    game.touch(player_id);
    send_game_state(game, msg);
}

bool Server::try_read_msg_type(msg_type_t &msg_type, const Message &msg) const
{
    if (!check_message_field(cf::MsgType::OFFSET, sizeof(msg_type_t), msg))
        return false;

    msg_type = read_uint8(msg.buffer, cf::MsgType::OFFSET);

    return true;
}

bool Server::try_read_game(GameMap::iterator &it, const Message &msg)
{
    if (!check_message_field(cf::GameID::OFFSET, sizeof(game_id_t), msg))
        return false;

    game_id_t game_id = read_uint32(msg.buffer, cf::GameID::OFFSET);
    it = games.find(game_id);

    if (it == games.end()) {
        send_wrong_msg(cf::GameID::OFFSET, msg);
        return false;
    }

    return true;
}

bool Server::try_read_player_id(player_id_t &player_id, const GameState &game,
                                const Message &msg) const
{
    if (!check_message_field(cf::PlayerID::OFFSET, sizeof(player_id_t), msg))
        return false;

    player_id = read_uint32(msg.buffer, cf::PlayerID::OFFSET);
    if (game.player_a_id != player_id && game.player_b_id != player_id) {
        send_wrong_msg(cf::PlayerID::OFFSET, msg);
        return false;
    }

    return true;
}

bool Server::check_message_length(size_t correct_length,
                                  const Message &msg) const
{
    if (msg.buffer.size() == correct_length)
        return true;

    size_t error_offset = min(msg.buffer.size(), correct_length);
    send_wrong_msg(error_offset, msg);

    return false;
}

bool Server::check_message_field(size_t field_offset, size_t field_size,
                                 const Message &msg) const
{
    if (msg.buffer.size() < field_offset + field_size) {
        send_wrong_msg(msg.buffer.size(), msg);
        return false;
    }

    return true;
}
