#include "game.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include <algorithm>
#include <chrono>

using std::all_of;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;

GameState::GameState(game_id_t id, player_id_t p_a_id, idx_t max_p,
                     const buffer_t &initial_row)
    : game_id(id), player_a_id(p_a_id), player_b_id(0),
      status(GameStatus::WAITING_FOR_OPPONENT), max_pawn(max_p),
      pawn_row(initial_row), last_activity_a(steady_clock::now()),
      last_activity_b(steady_clock::now())
{
}

bool GameState::process_timeouts(int timeout_sec) noexcept
{
    auto now = steady_clock::now();
    auto duration_a = duration_cast<seconds>(now - last_activity_a).count();
    auto duration_b = duration_cast<seconds>(now - last_activity_b).count();

    if (status == GameStatus::TURN_A || status == GameStatus::TURN_B) {
        bool a_timed_out = duration_a >= timeout_sec;
        bool b_timed_out = duration_b >= timeout_sec;

        if (a_timed_out || b_timed_out) {
            status = (duration_a >= duration_b) ? GameStatus::WIN_B
                                                : GameStatus::WIN_A;
            touch(player_a_id);
            duration_a = 0;
        }
    } else {
        auto min_duration = duration_a;
        if (player_b_id != 0)
            min_duration = std::min(duration_a, duration_b);

        return min_duration >= timeout_sec;
    }

    return false;
}

void GameState::touch(player_id_t player_id) noexcept
{
    if (player_id == player_a_id)
        last_activity_a = steady_clock::now();

    if (player_id == player_b_id)
        last_activity_b = steady_clock::now();
}

bool GameState::is_their_turn(player_id_t player_id) const noexcept
{
    return (status == GameStatus::TURN_A && player_id == player_a_id) ||
           (status == GameStatus::TURN_B && player_id == player_b_id);
}

void GameState::knock_pawns(idx_t pawn_idx, int count) noexcept
{
    size_t start_idx = pawn_idx;
    size_t end_idx = start_idx + count;

    if (end_idx - 1 > max_pawn)
        return;

    for (size_t i = start_idx; i < end_idx; ++i) {
        auto [byte_idx, bit_idx] = find_bit_idxs(i);
        if ((pawn_row[byte_idx] & (1 << bit_idx)) == 0)
            return;
    }

    for (size_t i = start_idx; i < end_idx; ++i) {
        auto [byte_idx, bit_idx] = find_bit_idxs(i);
        pawn_row[byte_idx] &= ~(1 << bit_idx);
    }

    end_turn();
}

void GameState::end_turn() noexcept
{
    bool no_pawns_left = all_of(pawn_row.begin(), pawn_row.end(),
                                [](auto byte) { return byte == 0; });

    if (no_pawns_left) {
        status = status == GameStatus::TURN_A ? GameStatus::WIN_A
                                              : GameStatus::WIN_B;
    } else {
        status = status == GameStatus::TURN_A ? GameStatus::TURN_B
                                              : GameStatus::TURN_A;
    }
}
