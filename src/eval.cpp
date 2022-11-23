#include <cassert>
#include "bitops.h"
#include "eval.h"
#include "magic.h"
#include "movegen.h"
#include "pst.h"

int eval::position(board& pos)
{
	int sum[2][2]{};

	pieces(pos, sum);
	pawns(pos, sum);

	sum[mg][white] += tempo_bonus[pos.turn];

	assert(pos.phase >= 0);
	const int phase = pos.phase <= phase_max ? pos.phase : phase_max;
	const int& weight = phase_weight[phase];
	const int mg_score = sum[mg][white] - sum[mg][black];
	const int eg_score = sum[eg][white] - sum[eg][black];

	int fading = 40;
	if (pos.half_moves > 60)
		fading -= pos.half_moves - 60;

	return negate[pos.turn] * ((mg_score * weight + eg_score * (256 - weight)) >> 8) * fading / 40;
}

void eval::init()
{
	for (int i = 0; i <= phase_max; ++i)
	{
		phase_weight[i] = i * 256 / phase_max;
	}

	for (int p = pawn; p <= king; ++p)
	{
		for (int s = mg; s <= eg; ++s)
		{
			for (int i = 0; i < 64; ++i)
			{
				pst[white][p][s][63 - i] += value[s][p];
				pst[black][p][s][i] = pst[white][p][s][63 - i];
			}
		}
	}

	for (int i = 0; i < 64; ++i)
	{
		passed_pawn[black][i] = passed_pawn[white][63 - i];
	}

	for (int i = 8; i < 56; ++i)
	{
		uint64_t files = file[i & 7];
		if (i % 8) files |= file[i - 1 & 7];
		if ((i - 7) % 8) files |= file[i + 1 & 7];

		front_span[white][i] = files & ~((1ULL << (i + 2)) - 1);
		front_span[black][i] = files & ((1ULL << (i - 1)) - 1);
	}
}

void eval::pawns(const board& pos, int sum[][2])
{
	for (int col = white; col <= black; ++col)
	{
		const int not_col = col ^ 1;
		uint64_t pawns = pos.pieces[pawn] & pos.side[col];
		while (pawns)
		{
			const auto sq = lsb(pawns);

			sum[mg][col] += pst[not_col][pawn][mg][sq];
			sum[eg][col] += pst[not_col][pawn][eg][sq];

			if (!(front_span[col][sq] & pos.pieces[pawn] & pos.side[not_col]))
			{
				if (const auto idx = sq & 7; !(file[idx] & front_span[col][sq] & pos.pieces[pawn]))
				{
					int mg_bonus = passed_pawn[not_col][sq];
					int eg_bonus = mg_bonus;

					if (const uint64_t blocked_path = file[idx] & front_span[col][sq] & pos.side[not_col])
					{
						const int blocker_cnt = popcnt(blocked_path);
						mg_bonus /= blocker_cnt + 2;
						eg_bonus /= blocker_cnt + 1;
					}

					const uint64_t pieces_behind = file[idx] & front_span[not_col][sq];

					if (const uint64_t majors = (pos.pieces[rook] | pos.pieces[queen]) & pos.side[col];
						(pieces_behind & majors) && !(pieces_behind & (pos.side[both] ^ majors)))
					{
						mg_bonus += major_behind_pp[mg];
						eg_bonus += major_behind_pp[eg];
					}

					sum[mg][col] += mg_bonus;
					sum[eg][col] += eg_bonus;
				}
			}
			pawns &= pawns - 1;
		}
	}
}

void eval::pieces(board& pos, int sum[][2])
{
	const int old_turn = pos.turn;

	for (int col = white; col <= black; ++col)
	{
		pos.turn = col;
		const int not_col = col ^ 1;
		int att_cnt = 0;
		int att_sum = 0;

		movegen::legal init;
		movegen::legal::pin_down(pos);

		const uint64_t king_zone = movegen::king_table[pos.king_sq[col ^ 1]];

		uint64_t pieces = pos.side[col] & pos.pieces[bishop];
		while (pieces)
		{
			const auto sq = lsb(pieces);

			sum[mg][col] += pst[not_col][bishop][mg][sq];
			sum[eg][col] += pst[not_col][bishop][eg][sq];

			const uint64_t targets
			{
				movegen::slider_attacks(bishop_slider, sq, pos.side[both] & ~(pos.pieces[queen] & pos.side[col]))
				& ~(pos.side[col] & pos.pieces[pawn]) & movegen::pinned[sq]
			};

			if (const uint64_t streak_king = targets & ~pos.side[both] & king_zone)
			{
				att_cnt += 1;
				att_sum += king_threat[bishop] * popcnt(streak_king);
			}

			if (pieces & (pieces - 1))
			{
				sum[mg][col] += bishop_pair[mg];
				sum[eg][col] += bishop_pair[eg];
			}

			const int cnt = popcnt(targets);
			sum[mg][col] += bishop_mob[mg][cnt];
			sum[eg][col] += bishop_mob[eg][cnt];

			pieces &= pieces - 1;
		}

		pieces = pos.side[col] & pos.pieces[rook];
		while (pieces)
		{
			const auto sq = lsb(pieces);

			sum[mg][col] += pst[not_col][rook][mg][sq];
			sum[eg][col] += pst[not_col][rook][eg][sq];

			const uint64_t targets
			{
				movegen::slider_attacks(rook_slider, sq, pos.side[both] & ~((pos.pieces[queen] | pos.pieces[rook]) & pos.side[col]))
				& ~(pos.side[col] & pos.pieces[pawn]) & movegen::pinned[sq]
			};

			if (const uint64_t streak_king = targets & ~pos.side[both] & king_zone)
			{
				att_cnt += 1;
				att_sum += king_threat[rook] * popcnt(streak_king);
			}

			if (!(file[sq & 7] & pos.pieces[pawn] & pos.side[col]))
			{
				sum[mg][col] += rook_open_file;

				if (!(file[sq & 7] & pos.pieces[pawn]))
				{
					sum[mg][col] += rook_open_file;
				}
			}

			const int cnt = popcnt(targets);
			sum[mg][col] += rook_mob[mg][cnt];
			sum[eg][col] += rook_mob[eg][cnt];

			pieces &= pieces - 1;
		}

		pieces = pos.side[col] & pos.pieces[queen];
		while (pieces)
		{
			const auto sq = lsb(pieces);

			sum[mg][col] += pst[not_col][queen][mg][sq];
			sum[eg][col] += pst[not_col][queen][eg][sq];

			const uint64_t targets
			{
				(movegen::slider_attacks(rook_slider, sq, pos.side[both]) | movegen::slider_attacks(bishop_slider, sq, pos.side[both]))
				& ~(pos.side[col] & pos.pieces[pawn]) & movegen::pinned[sq]
			};

			if (const uint64_t streak_king = targets & ~pos.side[both] & king_zone)
			{
				att_cnt += 1;
				att_sum += king_threat[queen] * popcnt(streak_king);
			}

			const int cnt = popcnt(targets);
			sum[mg][col] += queen_mob[mg][cnt];
			sum[eg][col] += queen_mob[eg][cnt];

			pieces &= pieces - 1;
		}

		const uint64_t pawn_att = attack::by_pawns(pos, col ^ 1) & ~pos.side[both];
		pieces = pos.side[col] & pos.pieces[knight];
		while (pieces)
		{
			const auto sq = lsb(pieces);

			sum[mg][col] += pst[not_col][knight][mg][sq];
			sum[eg][col] += pst[not_col][knight][eg][sq];

			const uint64_t targets
			{
				movegen::knight_table[sq]
				& ~(pos.side[col] & pos.pieces[pawn]) & ~pawn_att & movegen::pinned[sq]
			};

			if (const uint64_t streak_king = targets & ~pos.side[both] & king_zone)
			{
				att_cnt += 1;
				att_sum += king_threat[knight] * popcnt(streak_king);
			}

			if (targets & pos.pieces[knight] & pos.side[col])
			{
				sum[mg][col] += knights_connected[mg];
				sum[eg][col] += knights_connected[eg];
			}

			const int cnt = popcnt(targets);
			sum[mg][col] += knight_mob[mg][cnt];
			sum[eg][col] += knight_mob[eg][cnt];

			pieces &= pieces - 1;
		}

		sum[mg][col] += pst[not_col][king][mg][pos.king_sq[col]];
		sum[eg][col] += pst[not_col][king][eg][pos.king_sq[col]];

		const int score = king_safety_w[att_cnt & 7] * att_sum / 100;
		sum[mg][col] += score;
		sum[eg][col] += score;
	}

	pos.turn = old_turn;
}
