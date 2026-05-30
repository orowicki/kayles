#include <arpa/inet.h>
#include <cerrno>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "client.h"
#include "client_config.h"
#include "protocol.h"
#include "utils.h"

using std::cout;
using std::hex;
using std::memcpy;
using std::runtime_error;
using std::setfill;
using std::setw;
using std::string;
using std::to_string;
using std::to_underlying;
using std::uppercase;

namespace sf = server::field;
namespace sm = server::msg;

namespace
{

void print_error_response(const buffer_t &buf)
{
    idx_t error_index = read_uint8(buf, sf::ErrorIndex::OFFSET);

    cout << "--- SERVER ERROR (WRONG_MSG) ---\n";
    cout << "Status:      WRONG_MSG ("
         << static_cast<int>(to_underlying(GameStatus::WRONG_MSG)) << ")\n";
    cout << "Error index: " << static_cast<int>(error_index) << '\n';

    cout << "Echoed data: ";
    for (size_t i = 0; i < sm::WrongMsg::SIZE; ++i)
        cout << hex << uppercase << setw(2) << setfill('0')
             << static_cast<int>(buf[i]) << ' ';

    cout << '\n';
}

void print_status(game_status_t status)
{
    cout << "Status:      ";
    switch (static_cast<GameStatus>(status)) {
        case GameStatus::WAITING_FOR_OPPONENT:
            cout << "WAITING_FOR_OPPONENT (0)\n";
            break;
        case GameStatus::TURN_A:
            cout << "TURN_A (1)\n";
            break;
        case GameStatus::TURN_B:
            cout << "TURN_B (2)\n";
            break;
        case GameStatus::WIN_A:
            cout << "WIN_A (3)\n";
            break;
        case GameStatus::WIN_B:
            cout << "WIN_B (4)\n";
            break;
        default:
            cout << "Unknown (" << static_cast<int>(status) << ")\n";
    }
}

void print_board(idx_t max_pawn, const buffer_t &buf)
{
    size_t expected_size = sf::Board::OFFSET + (max_pawn / BYTE_SIZE) + 1;
    if (buf.size() < expected_size) {
        cout << "Warning: Buffer is too small to contain the full board!\n";
        return;
    }

    cout << "Board:       ";
    for (int i = 0; i <= max_pawn; ++i) {
        auto [byte_idx, bit_idx] = find_bit_idxs(i);
        byte_idx += sf::Board::OFFSET;

        bool is_pawn_present = (buf[byte_idx] >> bit_idx) & 1;
        cout << (is_pawn_present ? '1' : '0');
    }
    cout << '\n';
}

void print_game_state(const buffer_t &buf)
{
    game_status_t status = read_uint8(buf, sf::Status::OFFSET);
    game_id_t game_id = read_uint32(buf, sf::GameID::OFFSET);
    player_id_t player_a_id = read_uint32(buf, sf::PlayerAID::OFFSET);
    player_id_t player_b_id = read_uint32(buf, sf::PlayerBID::OFFSET);
    idx_t max_pawn = read_uint8(buf, sf::MaxPawn::OFFSET);

    cout << "--- GAME STATE ---\n";
    cout << "Game ID:     " << game_id << '\n';
    cout << "Player A ID: " << player_a_id << '\n';
    cout << "Player B ID: "
         << (player_b_id == 0 ? "None (Waiting)" : to_string(player_b_id))
         << '\n';
    cout << "Max Pawn:    " << static_cast<int>(max_pawn) << '\n';

    print_status(status);
    print_board(max_pawn, buf);
}
} /* namespace */

Client::Client(const ClientConfig &config) : cfg(config), socket_fd(-1)
{
    setup_socket();
}

Client::~Client() noexcept
{
    if (socket_fd != -1)
        close(socket_fd);
}

void Client::setup_socket()
{
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    string port_str = to_string(cfg.port);

    if (getaddrinfo(cfg.address.c_str(), port_str.c_str(), &hints, &res) != 0)
        throw runtime_error("Failed to resolve server address: " + cfg.address);

    memcpy(&server_addr, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        throw runtime_error("Failed to create socket!");
    }

    struct timeval tv{};
    tv.tv_sec = cfg.timeout;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(socket_fd);
        throw runtime_error("Failed to set socket timeout (SO_RCVTIMEO)!");
    }
}

void Client::send_message()
{
    ssize_t sent_bytes = sendto(
        socket_fd, cfg.message.data(), cfg.message.size(), 0,
        reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr));

    if (sent_bytes < 0)
        throw runtime_error("Failed to send message to the server!");
}

void Client::receive_response()
{
    buffer_t buffer(BUFFER_SIZE);
    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

    while (true) {
        ssize_t received_bytes = recvfrom(
            socket_fd, buffer.data(), buffer.size(), 0,
            reinterpret_cast<struct sockaddr *>(&from_addr), &from_len);

        if (received_bytes < 0) {
            cout << "Timeout: server did not respond.\n";
            return;
        }

        if (from_addr.sin_addr.s_addr == server_addr.sin_addr.s_addr &&
            from_addr.sin_port == server_addr.sin_port) {

            buffer.resize(received_bytes);
            print_response(buffer);
            return;
        }
    }
}

void Client::print_response(const buffer_t &buf) noexcept
{
    if (buf.size() < sm::WrongMsg::SIZE) {
        cout << "Received invalid response (too short: " << buf.size()
             << " bytes).\n";
        return;
    }

    game_status_t status = read_uint8(buf, sf::Status::OFFSET);

    if (status == to_underlying(GameStatus::WRONG_MSG)) {
        print_error_response(buf);
    } else {
        if (buf.size() < sm::GameState::MIN_SIZE) {
            cout << "Received truncated response (" << buf.size()
                 << " bytes).\n";
        } else {
            print_game_state(buf);
        }
    }
}

void Client::run()
{
    send_message();
    receive_response();
}
