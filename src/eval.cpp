#include <cassert>
#include "bitops.h"
#include "eval.h"
#include "game.h"
#include "pst.h"

namespace
{
	constexpr int phase_max{24};
	int phase_weight[phase_max + 1];
	constexpr int negate[]{1, -1};
	constexpr int tempo_bonus[]{ eval::tempo_bonus_white, 0 };
}

int eval::material(const board& pos)
{
	int count{0};
	for (int pce{pawns}; pce <= queens; ++pce)
	{
		count += piece_value[mg][pce] *
		(popcnt(pos.pieces[pce] & pos.side[white])
			- popcnt(pos.pieces[pce] & pos.side[black]));
	}
	return count;
}

int eval::piece_squares(const board& pos)
{
	int sum[2][2]{};

	for (int col{white}; col <= black; ++col)
	{
		uint64_t pieces{pos.side[col]};
		while (pieces)
		{
			const int pce{pos.piece_sq[lsb(pieces)]};
			sum[mg][col] += pst[pce][mg][col][63 - lsb(pieces)];
			sum[eg][col] += pst[pce][eg][col][63 - lsb(pieces)];
			pieces &= pieces - 1;
		}
	}

	assert(pos.phase >= 0);
	const int phase{pos.phase <= phase_max ? pos.phase : phase_max};
	const int weight{phase_weight[phase]};
	const int mg_score{sum[mg][white] - sum[mg][black]};
	const int eg_score{sum[eg][white] - sum[eg][black]};

	return (mg_score * weight + eg_score * (256 - weight)) >> 8;
}

void eval::pieces(board& pos, int sum[][2])
{
	const int old_turn{ pos.turn };

	for (int col{ white }; col <= black; ++col)
	{
		pos.turn = col;

		uint64_t pieces{ pos.side[col] & pos.pieces[bishops] };
		while (pieces)
		{
			if (pieces & (pieces - 1))
			{
				sum[mg][col] += bishop_pair[mg];
				sum[eg][col] += bishop_pair[eg];
			}
			pieces &= pieces - 1;
		}
	}
	pos.turn = old_turn;
}

void eval::init_phases()
{
	for (int i{ 0 }; i <= phase_max; ++i)
		phase_weight[i] = i * 256 / phase_max;
}

int eval::position(board& pos)
{
	int sum[2][2]{ };
	pieces(pos, sum);
	sum[mg][white] += tempo_bonus[pos.turn];
	return (material(pos) + piece_squares(pos)) * negate[pos.turn];
}