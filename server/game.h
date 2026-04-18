#ifndef GAME_H
#define GAME_H

#include "../common/protocol.h"
#include <chrono>
#include <cstdint>

struct GameState {
    game_id_t game_id;
    player_id_t player_a_id;
    player_id_t player_b_id;
    GameStatus status;
    idx_t max_pawn;
    buffer_t pawn_row;

    std::chrono::steady_clock::time_point last_activity_a;
    std::chrono::steady_clock::time_point last_activity_b;

    GameState(game_id_t id, player_id_t p_a_id, idx_t max_p,
              const buffer_t &initial_row);

    bool process_timeouts(int server_timeout_seconds) noexcept;

    bool is_their_turn(player_id_t player_id) const noexcept;

    void touch(player_id_t player_id) noexcept;

    void knock_pawns(idx_t pawn_idx, int count) noexcept;

private:
    void end_turn() noexcept;
};

#endif /* GAME_H */
