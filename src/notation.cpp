#include <cassert>
#include "movegen.h"
#include "bitops.h"
#include "notation.h"

uint64_t notation::conv_to_bb(const std::string& sq)
{
	return 1ULL << conv_to_int(sq);
}

uint8_t notation::to_flag(const char promo, const board& pos, const uint64_t& from_64, const uint64_t& to_64)
{
	assert(from_64 != 0 && to_64 != 0);

	const int from = lsb(from_64);
	const int to = lsb(to_64);

	auto flag = pos.piece_sq[to];

	if (const auto pce = pos.piece_sq[from]; pce == pawn)
	{
		if (~pos.side[both] & to_64 && abs(from - to) % 8 != 0)
			flag = enpassant;

		if (promo == 'q') flag = promo_queen;
		else if (promo == 'r') flag = promo_rook;
		else if (promo == 'n') flag = promo_knight;
		else if (promo == 'b') flag = promo_bishop;
	}

	else if (pce == king)
	{
		if (from == e1 && to == g1) flag = white_kingside;
		else if (from == e1 && to == c1) flag = white_queenside;
		else if (from == e8 && to == g8) flag = black_kingside;
		else if (from == e8 && to == c8) flag = black_queenside;
	}
	else
		assert(pce > pawn && pce < king);

	return flag;
}

int notation::conv_to_int(const std::string& sq)
{
	assert(sq.size() == 2U);
	assert(isdigit(static_cast<unsigned>(sq.back())));
	assert(!isdigit(static_cast<unsigned>(sq.front())));

	int shift = 'h' - sq.front();
	shift += (sq.back() - '1') * 8;

	assert(shift >= 0);
	assert(shift <= 63);

	return shift;
}

std::string notation::to_promotion(const uint8_t flag)
{
	assert(flag <= 15);
	std::string promo = " ";

	if (flag >= 12)
	{
		if (flag == promo_rook)
			promo = "r ";
		else if (flag == promo_knight)
			promo = "n ";
		else if (flag == promo_bishop)
			promo = "b ";
		else if (flag == promo_queen)
			promo = "q ";
		else
			assert(false);
	}
	return promo;
}

std::string notation::conv_to_san(board& pos, const uint64_t& from_64, const uint64_t& to_64, uint8_t flag)
{
	std::string move;
	movegen gen(pos, all);
	assert(gen.move_cnt != 0);

	int from = lsb(from_64);
	int to = lsb(to_64);

	if (flag == white_kingside || flag == black_kingside)
	{
		assert(from == e1 && to == g1 || from == e8 && to == g8);
		move = "O-O";
	}

	else if (flag == white_queenside || flag == black_queenside)
	{
		assert(from == e1 && to == c1 || from == e8 && to == c8);
		move = "O-O-O";
	}
	else
	{
		uint64_t pce = 0;
		std::string sq1_str = conv_to_str(from_64);

		if (auto pc = pos.piece_sq[from]; pc != pawn)
		{
			move += piece_char[pc - 1];
			pce = pos.pieces[pc];
		}
		else if (to_64 & pos.side[both])
		{
			assert(from_64 & pos.pieces[pawn]);
			move = sq1_str.front();
		}

		uint64_t possible = 0;
		for (int cnt = 0; cnt < gen.move_cnt; ++cnt)
		{
			if (1ULL << to_sq2(gen.movelist[cnt]) == to_64 && pce & 1ULL << to_sq1(gen.movelist[cnt]))
				possible |= 1ULL << to_sq1(gen.movelist[cnt]);
		}

		if (possible & possible - 1)
		{
			auto rank_idx = sq1_str.back() - '1';
			auto file_idx = 'h' - sq1_str.front();

			assert(1ULL << (file_idx + 8 * rank_idx) == from_64);

			if (popcnt(possible & rank[rank_idx]) >= 2)
				move += sq1_str.front();
			else if (popcnt(possible & file[file_idx]) >= 2)
				move += sq1_str.back();
			else
				move += sq1_str.front();
		}

		if (to_64 & pos.side[both])
			move += "x";
		move += conv_to_str(to_64);

		if (to_promotion(flag) != " ")
			move += to_promotion(flag).front();
	}

	uint16_t move_temp{encode(from, to, flag)};

	board save(pos);
	assert(gen.in_list(move_temp));

	pos.new_move(move_temp);

	if (uint64_t enemy_king = pos.side[pos.turn ^ 1] & pos.pieces[king]; !movegen::check(pos, pos.turn ^ 1, enemy_king))
		move += "+";

	pos = save;
	return move;
}

std::string notation::conv_to_str(const uint64_t& sq)
{
	return conv_to_str(lsb(sq));
}

std::string notation::conv_to_str(const int sq)
{
	std::string str;
	str += 'h' - static_cast<char>(sq & 7);
	str += '1' + static_cast<char>(sq >> 3);

	return str;
}
