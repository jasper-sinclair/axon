#include <cassert>
#include "sort.h"

int sort::mvv_lva(const board& pos, const uint16_t move)
{
	assert(to_flag(move) <= enpassant && to_flag(move) != no_piece);
	return p_value[to_flag(move)] * 100 - p_value[pos.piece_sq[to_sq1(move)]];
}

int sort::mvv_lva_promo(const board& pos, const uint16_t move)
{
	assert(to_flag(move) >= promo_rook);
	const int victim = p_value[pos.piece_sq[to_sq2(move)]] + p_value[to_flag(move) - 11] - 2;
	return victim * 100 - p_value[to_flag(move) - 11];
}

void sort::sort_capt(const board& pos, const movegen& gen)
{
	for (int nr = 0; nr < gen.capt_cnt; ++nr)
		score_list[nr] = base_score + mvv_lva(pos, gen.movelist[nr]);

	for (int nr = gen.capt_cnt; nr < gen.capt_cnt + gen.promo_cnt; ++nr)
		score_list[nr] = base_score + mvv_lva_promo(pos, gen.movelist[nr]);
}

void sort::sort_main(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64], uint16_t killer[][2], const int depth)
{
	static_assert(no_piece << 12 == 0x6000, "killer encoding");
	move_cnt = gen.move_cnt;

	sort_capt(pos, gen);

	for (int i = gen.capt_cnt + gen.promo_cnt; i < move_cnt; ++i)
	{
		score_list[i] = history[pos.turn][pos.piece_sq[to_sq1(gen.movelist[i])]][to_sq2(gen.movelist[i])];
	}

	for (int slot = 0; slot < 2; ++slot)
	{
		auto killer_move = killer[depth][slot];

		if (const auto pce = to_flag(killer_move); pce <= king)
		{
			killer_move = (killer_move & 0xfff) | 0x6000;
			if (pce != pos.piece_sq[to_sq1(killer_move)])
				continue;
		}

		for (int i = gen.capt_cnt + gen.promo_cnt; i < move_cnt; ++i)
		{
			if (gen.movelist[i] == killer_move)
			{
				score_list[i] = base_score + 2 - slot;
				break;
			}
		}
	}

	if (best_move != nullptr)
		score_list[best_move - gen.movelist] = maxscore;
}

uint16_t sort::next(const movegen& gen)
{
	int nr = -1;

	for (int i = 0, best_score = 0; i < move_cnt; ++i)
	{
		if (score_list[i] > best_score)
		{
			best_score = score_list[i];
			nr = i;
		}
	}

	if (nr != -1)
	{
		score_list[nr] = 0;
		return gen.movelist[nr];
	}
	return 0;
}

void sort::sort_qsearch(const board& pos, const movegen& gen)
{
	assert(gen.capt_cnt + gen.promo_cnt == gen.move_cnt);
	move_cnt = gen.move_cnt;

	sort_capt(pos, gen);
}

void sort::sort_root(const board& pos, const movegen& gen, const uint16_t* best_move, int history[][6][64])
{
	move_cnt = gen.move_cnt;

	sort_capt(pos, gen);

	for (int i = gen.capt_cnt + gen.promo_cnt; i < move_cnt; ++i)
	{
		score_list[i] = history[pos.turn][pos.piece_sq[to_sq1(gen.movelist[i])]][to_sq2(gen.movelist[i])];
	}

	if (best_move != nullptr)
		score_list[best_move - gen.movelist] = maxscore;
}
