#pragma once
#include "movegen.h"
#include "board.h"
#include "common.h"

class sort
{
public:
	sort() = default;

	sort(const board& pos, const movegen& gen)
	{
		sort_qsearch(pos, gen);
	}

	sort(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64])
	{
		sort_root(pos, gen, best_move, history);
	}

	sort(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64], uint16_t killer[][2], const int depth)
	{
		sort_main(pos, gen, best_move, history, killer, depth);
	}

	int move_cnt = 0;
	int score_list[max_movegen]{};
	static int mvv_lva(const board& pos, uint16_t move);
	static int mvv_lva_promo(const board& pos, uint16_t move);
	uint16_t next(const movegen& gen);
	void sort_capt(const board& pos, const movegen& gen);
	void sort_main(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64], uint16_t killer[][2], int depth);
	void sort_qsearch(const board& pos, const movegen& gen);
	void sort_root(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64]);
};

inline constexpr int p_value[]{1, 5, 3, 3, 9, 0, 0, 1};
inline constexpr int maxscore = 0x7fffffff;
inline constexpr int base_score = 1 << 30;
