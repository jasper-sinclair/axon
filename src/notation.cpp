#include <cassert>
#include "bitops.h"
#include "movegen.h"
#include "notation.h"

std::string notation::convert_to_san(board& pos, const uint64_t& from_64, const uint64_t& to_64, const uint8_t flag)
{
	std::string move;
	const movegen gen(pos, all);
	assert(gen.move_cnt != 0);

	const int from{ (lsb(from_64)) };
	const int to{ (lsb(to_64)) };

	if (flag == white_kingside || flag == black_kingside)
	{
		assert((from_64 & 0x800000000000008) && (to_64 & 0x200000000000002));
		move = "O-O";
	}
	else if (flag == white_queenside || flag == black_queenside)
	{
		assert((from_64 & 0x800000000000008) && (to_64 & 0x2000000000000020));
		move = "O-O-O";
	}
	else
	{
		uint64_t pce{ 0 };
		const std::string sq1_str{ conv_to_str(from_64) };

		if (const auto pc{ pos.piece_sq[from] }; pc != pawns)
		{
			move += piece_char[pc - 1];
			pce = pos.pieces[pc];
		}
		else if (to_64 & pos.side[both])
		{
			assert(from_64 & pos.pieces[pawns]);
			move = sq1_str.front();
		}

		uint64_t possible{ 0 };
		for (int cnt{ 0 }; cnt < gen.move_cnt; ++cnt)
		{
			if (1ULL << to_sq2(gen.movelist[cnt]) == to_64 && pce & 1ULL << to_sq1(gen.movelist[cnt]))
				possible |= 1ULL << to_sq1(gen.movelist[cnt]);
		}

		if (possible & possible - 1)
		{
			const auto r{ sq1_str.back() - '1' };
			const auto f{ 'h' - sq1_str.front() };

			assert(1ULL << f + 8 * r == from_64);

			if (popcnt(possible & rank_1 << r) >= 2)
				move += sq1_str.front();
			else if (popcnt(possible & file_h << f) >= 2)
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

	const uint16_t move_temp{ encode(from, to, flag) };

	const board save(pos);
	assert(gen.in_list(move_temp));

	pos.new_move(move_temp);

	if (const uint64_t king_enemy{ pos.side[pos.turn ^ 1] & pos.pieces[kings] }; !movegen::check(pos, pos.turn ^ 1, king_enemy))
		move += "+";

	pos = save;
	return move;
}

int notation::convert_to_int(const std::string& sq)
{
	assert(sq.size() == 2U);
	assert(isdigit(uint16_t(sq.back())));
	assert(!isdigit(uint16_t(sq.front())));

	int shift{ 'h' - sq.front() };
	shift += (sq.back() - '1') * 8;

	assert(shift >= 0);
	assert(shift <= 63);

	return shift;
}

std::string notation::to_promotion(const uint8_t flag)
{
	assert(flag <= 15);
	std::string promo{ " " };

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

uint8_t notation::to_flag(const char promo, const board& pos, const uint64_t& from_64, const uint64_t& to_64)
{
	assert(from_64 != 0 && to_64 != 0);

	const int from{ (lsb(from_64)) };
	const int to{ (lsb(to_64)) };

	auto flag{ pos.piece_sq[to] };

	if (const auto pce{ pos.piece_sq[from] }; pce == pawns)
	{
		if (~pos.side[both] & to_64 && abs(from - to) % 8 != 0)
			flag = enpassant;

		if (promo == 'q') flag = promo_queen;
		else if (promo == 'r') flag = promo_rook;
		else if (promo == 'n') flag = promo_knight;
		else if (promo == 'b') flag = promo_bishop;
	}

	else if (pce == kings)
	{
		if (from == e1 && to == g1) flag = white_kingside;
		else if (from == e1 && to == c1) flag = white_queenside;
		else if (from == e8 && to == g8) flag = black_kingside;
		else if (from == e8 && to == c8) flag = black_queenside;
	}
	else
		assert(pce > pawns && pce < kings);

	return static_cast<uint8_t>(flag);
}

uint64_t notation::conv_to_bb(const std::string& sq) { return 1ULL << convert_to_int(sq); }
std::string notation::conv_to_str(const uint64_t& sq) { return conv_to_str(lsb(sq)); }
std::string notation::conv_to_str(const int sq) { std::string str; str += 'h' - (sq & 7); str += '1' + (sq >> 3); return str; }
