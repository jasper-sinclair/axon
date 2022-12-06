#pragma once
#include "board.h"
#include "timeman.h"

namespace analysis
{
	void reset();
}

namespace search
{
	int alphabeta(board& pos, int ply, int depth, int alpha, int beta);
	int qsearch(board& pos, int alpha, int beta);
	uint16_t id_frame(board& pos, timemanager& chrono);
	void root_alphabeta(board& pos, uint16_t pv[], int& best_score, int ply);
}

inline struct results
{
	double factor;
	double prev_nodes;
	int count;
	uint64_t all_nodes;
} ebf;

bool is_check(const board& pos);
bool is_promo(uint16_t move);
bool time_is_up();
constexpr auto last = max_depth - 1;
constexpr int max_history = 1 << 30;
inline bool no_pruning[max_depth]{ false };
inline int history[2][6][64]{};
inline movegen root;
inline uint16_t killer[max_depth][2]{};
inline uint16_t pv_evol[max_depth][max_depth];
inline uint64_t hashlist[max_period];
void update_heuristics(const board& pos, uint16_t move, int ply, int depth);
void update_history(const board& pos, uint16_t move, int pce, int ply);
void update_killer(uint16_t move, int depth);

inline struct time_management
{
	int max = 0;
	timer manage;
} spare_time;

inline bool is_check(const board& pos)
{
	return movegen::check(pos, pos.turn, pos.pieces[king] & pos.side[pos.turn]) == 0ULL;
}

inline bool is_promo(const uint16_t move)
{
	static_assert(promo_rook == 12, "promo encoding");
	return to_flag(move) >= promo_rook;
}

inline bool time_is_up()
{
	return spare_time.manage.elapsed() >= spare_time.max;
}

inline void update_heuristics(const board& pos, const uint16_t move, const int ply, const int depth)
{
	if (const auto flag = to_flag(move); flag == no_piece)
	{
		const auto pce = pos.piece_sq[to_sq1(move)];
		const auto new_kill = (move & 0xfff) | (pce << 12);
		update_history(pos, move, pce, ply);
		update_killer(static_cast<uint16_t>(new_kill), depth);
	}

	else if (flag >= white_kingside && flag <= black_queenside)
	{
		update_history(pos, move, king, ply);
		update_killer(move, depth);
	}
}

inline void update_history(const board& pos, const uint16_t move, const int pce, const int ply)
{
	auto* h = &history[pos.turn][pce][to_sq2(move)];
	*h += ply * ply;

	if (*h > max_history)
	{
		for (auto& i : history) for (auto& j : i) for (auto& k : j) k /= 2;
	}
}

inline void update_killer(const uint16_t move, const int depth)
{
	if (move == killer[depth][0] || move == killer[depth][1])
		return;

	killer[depth][1] = killer[depth][0];
	killer[depth][0] = move;
}
