#include <cassert>
#include "board.h"
#include "bitops.h"
#include "hash.h"
#include "notation.h"
#include "random.h"

uint64_t board::nodes;

void board::clear()
{
	for (auto& p : pieces) p = 0ULL;
	for (auto& s : side) s = 0ULL;
	for (auto& p : piece_sq) p = no_piece;

	moves = 0;
	half_moves = 0;
	castle_rights = 0;
	ep_square = 0ULL;
	turn = white;
	phase = 0;
}

bool board::lone_king() const
{
	return (pieces[king] | pieces[pawn]) == side[both];
}

void board::new_move(const uint16_t move)
{
	nodes += 1;
	moves += 1;
	half_moves += 1;

	const auto from = to_sq1(move);
	const auto to = to_sq2(move);
	assert(from >= 0 && from < 64);
	assert(to >= 0 && to < 64);

	const uint64_t from_64 = 1ULL << from;
	const uint64_t to_64 = 1ULL << to;

	const auto flag = to_flag(move);
	const auto pce = piece_sq[from];
	const auto victim = piece_sq[to];

	assert(flag == victim || flag >= 7);
	assert(pce != no_piece);

	const auto saved_castl_r = castle_rights;
	rook_moved(to_64, to);
	rook_moved(from_64, from);

	if (victim != no_piece)
	{
		assert(to_64 & side[turn ^ 1]);
		half_moves = 0;

		side[turn ^ 1] &= ~to_64;
		pieces[victim] &= ~to_64;

		phase -= phase_value[victim];
		key ^= random_u64_number();

		assert(phase >= 0);
	}
	else if (pce == pawn)
	{
		half_moves = 0;
		if (flag == enpassant)
		{
			assert(ep_square != 0);

			const uint64_t capt = shift(ep_square, ppush[turn ^ 1]);
			assert(capt & pieces[pawn] & side[turn ^ 1]);

			pieces[pawn] &= ~capt;
			side[turn ^ 1] &= ~capt;

			const auto sq_old = static_cast<uint16_t>(lsb(capt));

			assert(piece_sq[sq_old] == pawn);
			piece_sq[sq_old] = no_piece;

			key ^= random_u64_number();
		}
	}

	ep_square = 0ULL;
	if (pce == pawn && abs(from - to) == 16)
	{
		ep_square = shift(from_64, ppush[turn]);

		if (const auto file_idx = from & 7; pieces[pawn] & side[turn ^ 1] & ep_flank[turn][file_idx])
			key ^= random_u64_number();
	}

	pieces[pce] ^= from_64;
	side[turn] ^= from_64;
	pieces[pce] |= to_64;
	side[turn] |= to_64;
	piece_sq[to] = pce;
	piece_sq[from] = no_piece;

	key ^= random_u64_number();
	key ^= random_u64_number();

	if (flag >= 8)
	{
		if (flag <= 11)
		{
			switch (flag)
			{
			case white_kingside:
				pieces[rook] ^= 0x1, side[turn] ^= 0x1;
				pieces[rook] |= 0x4, side[turn] |= 0x4;
				piece_sq[h1] = no_piece, piece_sq[f1] = rook;

				key ^= random_u64_number();
				key ^= random_u64_number();
				break;

			case black_kingside:
				pieces[rook] ^= 0x100000000000000, side[turn] ^= 0x100000000000000;
				pieces[rook] |= 0x400000000000000, side[turn] |= 0x400000000000000;
				piece_sq[h8] = no_piece, piece_sq[f8] = rook;

				key ^= random_u64_number();
				key ^= random_u64_number();
				break;

			case white_queenside:
				pieces[rook] ^= 0x80, side[turn] ^= 0x80;
				pieces[rook] |= 0x10, side[turn] |= 0x10;
				piece_sq[a1] = no_piece, piece_sq[d1] = rook;

				key ^= random_u64_number();
				key ^= random_u64_number();
				break;

			case black_queenside:
				pieces[rook] ^= 0x8000000000000000, side[turn] ^= 0x8000000000000000;
				pieces[rook] |= 0x1000000000000000, side[turn] |= 0x1000000000000000;
				piece_sq[a8] = no_piece, piece_sq[d8] = rook;

				key ^= random_u64_number();
				key ^= random_u64_number();
				break;

			default:
				assert(false);
			}
		}

		else
		{
			const int promo_p = flag - 11;

			assert(flag >= 12 && flag <= 15);
			assert(pieces[pawn] & to_64);
			assert(piece_sq[to] == pawn);

			pieces[pawn] ^= to_64;
			pieces[promo_p] |= to_64;
			piece_sq[to] = static_cast<uint8_t>(promo_p);

			key ^= random_u64_number();
			key ^= random_u64_number();

			phase += phase_value[promo_p];
		}
	}

	if (to_64 & pieces[king])
	{
		castle_rights &= castle_r[turn];
		king_sq[turn] = to;
	}

	if (saved_castl_r != castle_rights)
	{
		const auto changes = saved_castl_r ^ castle_rights;
		for (const unsigned char i : castleright)
		{
			if (changes & i)
				key ^= random_u64_number();
		}
	}

	turn ^= 1;
	key ^= is_turn[0];

	side[both] = side[white] | side[black];
	assert(side[both] == (side[white] ^ side[black]));
}

void board::null_move(uint64_t& ep_copy)
{
	key ^= is_turn[0];
	if (ep_square != 0)
	{
		if (const auto file_idx = lsb(ep_square) & 7; pieces[pawn] & side[turn ^ 1] & ep_flank[turn][file_idx])
			key ^= random_u64_number();
	}

	ep_copy = ep_square;
	ep_square = 0;
	half_moves += 1;
	moves += 1;
	nodes += 1;
	turn ^= 1;
}

void board::parse_fen(const std::string& fen)
{
	clear();
	int sq = 63;
	uint32_t focus = 0;
	assert(focus < fen.size());

	while (focus < fen.size() && fen[focus] != ' ')
	{
		assert(sq >= 0);

		if (fen[focus] == '/')
		{
			focus += 1;
			assert(focus < fen.size());
			continue;
		}
		if (isdigit(fen[focus]))
		{
			sq -= fen[focus] - '0';
			assert(fen[focus] - '0' <= 8 && fen[focus] - '0' >= 1);
		}
		else
		{
			for (int pce = pawn; pce <= king; ++pce)
			{
				for (int col = white; col <= black; ++col)
				{
					if (fen[focus] == p_char[col][pce])
					{
						pieces[pce] |= 1ULL << sq;
						side[col] |= 1ULL << sq;
						piece_sq[sq] = static_cast<uint8_t>(pce);

						phase += phase_value[pce];
						break;
					}
				}
			}
			sq -= 1;
		}
		focus += 1;
		assert(focus < fen.size());
	}

	side[both] = side[white] | side[black];
	assert(side[both] == (side[white] ^ side[black]));

	for (int col = white; col <= black; ++col)
	{
		king_sq[col] = lsb(pieces[king] & side[col]);
	}

	focus += 1;
	if (fen[focus] == 'w')
		turn = white;
	else if (fen[focus] == 'b')
		turn = black;

	focus += 2;
	while (focus < fen.size() && fen[focus] != ' ')
	{
		if (fen[focus] == '-');
		else if (fen[focus] == 'K')
			castle_rights |= castleright[white_short];
		else if (fen[focus] == 'Q')
			castle_rights |= castleright[white_long];
		else if (fen[focus] == 'k')
			castle_rights |= castleright[black_short];
		else if (fen[focus] == 'q')
			castle_rights |= castleright[black_long];
		focus += 1;
	}

	focus += 1;
	if (fen[focus] == '-')
		focus += 1;
	else
	{
		ep_square = notation::conv_to_bb(fen.substr(focus, 2));
		focus += 2;
	}

	if (focus >= fen.size() - 1)
		return;

	focus += 1;
	std::string half_move_c;
	while (focus < fen.size() && fen[focus] != ' ')
		half_move_c += fen[focus++];
	half_moves = stoi(half_move_c);

	focus += 1;
	std::string move_c;
	while (focus < fen.size() && fen[focus] != ' ')
		move_c += fen[focus++];
	moves = stoi(move_c);

	key = to_key(*this);
}

void board::rook_moved(const uint64_t& sq64, const uint16_t sq)
{
	if (sq64 & pieces[rook])
	{
		if (sq64 & side[white])
		{
			if (sq == h1) castle_rights &= ~castleright[white_short];
			else if (sq == a1) castle_rights &= ~castleright[white_long];
		}
		else
		{
			if (sq == h8) castle_rights &= ~castleright[black_short];
			else if (sq == a8) castle_rights &= ~castleright[black_long];
		}
	}
}

void board::undo_null_move(const uint64_t& ep_copy)
{
	key ^= is_turn[0];
	if (ep_copy != 0)
	{
		if (const auto file_idx = lsb(ep_copy) & 7; pieces[pawn] & side[turn ^ 1] & ep_flank[turn][file_idx])
			key ^= random_u64_number();
	}

	ep_square = ep_copy;
	half_moves -= 1;
	moves -= 1;
	turn ^= 1;
}
