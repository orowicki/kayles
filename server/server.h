#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

#include "game.h"
#include "server_config.h"

using GameMap = std::unordered_map<game_id_t, GameState>;

constexpr size_t BUFFER_SIZE = 2048;

class Server
{
public:
    explicit Server(const ServerConfig &config);
    ~Server();

    void run();

private:
    ServerConfig cfg;
    int socket_fd;
    game_id_t next_game_id;
    bool out_of_ids = false;

    GameMap games;

    void setup_socket();

    void cleanup_expired_games();

    void send_game_state(const GameState &game,
                         const Message &original_msg) const;

    void send_wrong_msg(size_t error_offset, const Message &original_msg) const;

    void handle_message(const Message &msg);

    void handle_join(const Message &msg);

    void handle_move(const Message &msg);

    void handle_keep_alive(const Message &msg);

    void handle_give_up(const Message &msg);

    bool try_read_msg_type(msg_type_t &msg_type, const Message &msg) const;

    bool try_read_game(GameMap::iterator &it, const Message &msg);

    bool try_read_player_id(player_id_t &player_id, const GameState &game,
                            const Message &msg) const;

    bool check_message_field(size_t field_offset, size_t field_size,
                             const Message &msg) const;

    bool check_message_length(size_t expected_length, const Message &msg) const;
};

#endif /* SERVER_H */
