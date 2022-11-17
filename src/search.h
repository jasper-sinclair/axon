#pragma once
#include "movegen.h"
#include "board.h"
#include "timeman.h"
#include "common.h"

namespace search
{
	uint16_t idd(board& pos, timemanager& chrono);
	void root_alphabeta(board& pos, uint16_t pv[], int& best_score, int ply);
	int alphabeta(board& pos, int ply, int depth, int alpha, int beta);
	int qsearch(board& pos, int alpha, int beta);
}

inline int max_time;
inline timer total_time;
inline bool time_is_up() { return total_time.elapsed() >= max_time; }

inline bool is_check(const board& pos) { return movegen::check(pos, pos.turn, pos.pieces[kings] & pos.side[pos.turn]) == 0ULL; }
inline bool is_promo(const uint16_t move) { return to_flag(move) < 12; }

inline movegen root;

constexpr auto last{max_depth - 1};
inline uint64_t hashlist[max_period];
inline bool no_pruning[max_depth]{false};
inline uint16_t pv_evol[max_depth][max_depth];
