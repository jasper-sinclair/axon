#include <algorithm>
#include <cassert>
#include "bitops.h"
#include "magic.h"
#include "movegen.h"

uint64_t movegen::check(const board& pos, const int turn, uint64_t squares)
{
	assert(turn == white || turn == black);

	const uint64_t k = pos.side[turn] & pos.pieces[king];
	uint64_t inquire = squares;

	while (inquire)
	{
		const auto sq = lsb(inquire);
		const uint64_t sq64 = 1ULL << sq;
		const uint64_t in_front[]{~(sq64 - 1), sq64 - 1};

		uint64_t att = slider_attacks(rook_slider, sq, pos.side[both] & ~k) & (pos.pieces[rook] | pos.pieces[queen]);
		att |= slider_attacks(bishop_slider, sq, pos.side[both] & ~k) & (pos.pieces[bishop] | pos.pieces[queen]);
		att |= knight_table[sq] & pos.pieces[knight];
		att |= king_table[sq] & pos.pieces[king];
		att |= king_table[sq] & pos.pieces[pawn] & slide_ray[bishop_slider][sq] & in_front[turn];
		att &= pos.side[turn ^ 1];

		if (att) squares ^= sq64;

		inquire &= inquire - 1;
	}
	return squares;
}

uint16_t* movegen::find(const uint16_t move)
{
	return std::find(movelist, movelist + move_cnt, move);
}

int movegen::gen_moves(const board& pos, const gentype type)
{
	ahead = static_cast<uint8_t>(pos.turn);
	back = static_cast<uint8_t>(pos.turn * 2);

	border_right = ~border[pos.turn ^ 1];
	border_left = ~border[pos.turn];
	pawn_rank = third_rank[pos.turn];

	friends = pos.side[pos.turn];
	enemies = pos.side[pos.turn ^ 1];
	fr_king = pos.pieces[king] & friends;

	legal init(pos);

	gen_type[captures] = enemies;
	gen_type[quiets] = ~pos.side[both];
	move_cnt = promo_cnt = capt_cnt = 0;

	king_moves(pos, captures);
	pawn_captures(pos);
	piece_moves(pos, captures);
	capt_cnt = move_cnt;

	pawn_promo(pos);
	promo_cnt = move_cnt - capt_cnt;

	if (type == all)
	{
		pawn_quiet(pos);
		piece_moves(pos, quiets);
		king_moves(pos, quiets);
	}

	return move_cnt;
}

void movegen::init()
{
	std::fill_n(pinned, 64, 0xffffffffffffffff);
}

bool movegen::in_list(const uint16_t move)
{
	return find(move) != movelist + move_cnt;
}

void movegen::king_moves(const board& pos, const gentype type)
{
	uint64_t targets = check(pos, pos.turn, king_table[pos.king_sq[pos.turn]] & gen_type[type]);
	while (targets)
	{
		movelist[move_cnt++] = encode(pos.king_sq[pos.turn], lsb(targets), pos.piece_sq[lsb(targets)]);
		targets &= targets - 1;
	}

	if (type == quiets && fr_king & 0x800000000000008)
	{
		const uint64_t rank_king = rank[pos.turn * 7];

		constexpr uint8_t rights_s[]{0x1, 0x10};
		constexpr uint8_t rights_l[]{0x4, 0x40};

		if (rights_s[pos.turn] & pos.castle_rights && !(pos.side[both] & 0x600000000000006 & rank_king)
			&& popcnt(check(pos, pos.turn, 0x0e0000000000000e & rank_king)) == 3)
		{
			constexpr uint32_t target[]{1, 57};
			movelist[move_cnt++] = encode(pos.king_sq[pos.turn], target[pos.turn], white_kingside + pos.turn * 2);
		}
		if (rights_l[pos.turn] & pos.castle_rights && !(pos.side[both] & 0x7000000000000070 & rank_king)
			&& popcnt(check(pos, pos.turn, 0x3800000000000038 & rank_king)) == 3)
		{
			constexpr uint32_t target[]{5, 61};
			movelist[move_cnt++] = encode(pos.king_sq[pos.turn], target[pos.turn], (white_queenside + pos.turn * 2));
		}
	}
}

void movegen::init_legal(const board& pos)
{
	const uint64_t in_front[]{~(fr_king - 1), fr_king - 1};
	const auto king_sq = pos.king_sq[pos.turn];
	assert(fr_king != 0ULL);

	uint64_t att = slider_attacks(rook_slider, king_sq, pos.side[both]) & (pos.pieces[rook] | pos.pieces[queen]);
	att |= slider_attacks(bishop_slider, king_sq, pos.side[both]) & (pos.pieces[bishop] | pos.pieces[queen]);
	att |= knight_table[king_sq] & pos.pieces[knight];
	att |= king_table[king_sq] & pos.pieces[pawn] & slide_ray[bishop_slider][king_sq] & in_front[pos.turn];
	att &= enemies;

	if (const int nr_att = popcnt(att); nr_att == 0)
	{
		legal_sq = 0xffffffffffffffff;
	}
	else if (nr_att == 1)
	{
		if (att & pos.pieces[knight] || att & pos.pieces[pawn])
		{
			legal_sq = att;
		}
		else
		{
			assert(att & pos.pieces[rook] || att & pos.pieces[bishop] || att & pos.pieces[queen]);
			const auto every_att = slider_attacks(rook_slider, king_sq, pos.side[both]) | slider_attacks(bishop_slider, king_sq, pos.side[both]);

			for (const auto dir : ray)
			{
				auto flood = fr_king;
				for (; !(flood & dir.border); flood |= shift(flood, dir.shift));

				if (flood & att)
				{
					legal_sq = flood & every_att;
					break;
				}
			}
		}
	}
	else
	{
		assert(nr_att == 2);
		legal_sq = 0ULL;
	}
}

void movegen::legal::pin_down(const board& pos)
{
	friends = pos.side[pos.turn];
	enemies = pos.side[pos.turn ^ 1];
	fr_king = pos.pieces[king] & friends;

	pin(pos);
}

void movegen::pawn_captures(const board& pos)
{
	uint64_t targets = shift(pos.pieces[pawn] & friends & border_left, cap_left[ahead]) & ~promo_rank;
	uint64_t targets_cap = targets & enemies & legal_sq;

	while (targets_cap)
	{
		uint64_t target = 1ULL << lsb(targets_cap);
		const auto to_sq = lsb(targets_cap);
		const auto from_sq = to_sq - cap_left[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, pos.piece_sq[to_sq]);

		targets_cap &= targets_cap - 1;
	}

	uint64_t target_ep = targets & pos.ep_square & shift(legal_sq, push[ahead]);
	if (target_ep)
	{
		const auto to_sq = lsb(target_ep);
		const auto from_sq = to_sq - cap_left[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target_ep &= pinned[from_sq];
		if (target_ep)
			movelist[move_cnt++] = encode(from_sq, to_sq, enpassant);
	}

	targets = shift(pos.pieces[pawn] & friends & border_right, cap_right[ahead]) & ~promo_rank;
	targets_cap = targets & enemies & legal_sq;

	while (targets_cap)
	{
		uint64_t target = 1ULL << lsb(targets_cap);
		const auto to_sq = lsb(targets_cap);
		const auto from_sq = to_sq - cap_right[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, pos.piece_sq[to_sq]);

		targets_cap &= targets_cap - 1;
	}

	target_ep = targets & pos.ep_square & shift(legal_sq, push[ahead]);
	if (target_ep)
	{
		const auto to_sq = lsb(target_ep);
		const auto from_sq = to_sq - cap_right[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target_ep &= pinned[from_sq];
		if (target_ep)
			movelist[move_cnt++] = encode(from_sq, to_sq, enpassant);
	}
}

void movegen::pawn_promo(const board& pos)
{
	uint64_t targets = shift(pos.pieces[pawn] & friends & border_left, cap_left[ahead]) & legal_sq & promo_rank & enemies;
	while (targets)
	{
		uint64_t target = 1ULL << lsb(targets);
		const auto to_sq = lsb(targets);
		const auto from_sq = to_sq - cap_left[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag = 15; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}

	targets = shift(pos.pieces[pawn] & friends & border_right, cap_right[ahead]) & legal_sq & promo_rank & enemies;
	while (targets)
	{
		uint64_t target = 1ULL << lsb(targets);
		const auto to_sq = lsb(targets);
		const auto from_sq = to_sq - cap_right[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag = 15; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}

	targets = shift(pos.pieces[pawn] & friends, push[ahead]) & ~pos.side[both] & legal_sq & promo_rank;
	while (targets)
	{
		uint64_t target = 1ULL << lsb(targets);
		const auto to_sq = lsb(targets);
		const auto from_sq = to_sq - push[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag = 15; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}
}

void movegen::pawn_quiet(const board& pos)
{
	const uint64_t pushes = shift(pos.pieces[pawn] & friends, push[ahead]) & ~pos.side[both] & ~promo_rank;
	uint64_t targets = pushes & legal_sq;

	while (targets)
	{
		uint64_t target = 1ULL << lsb(targets);
		const auto to_sq = lsb(targets);
		const auto from_sq = to_sq - push[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, no_piece);

		targets &= targets - 1;
	}

	uint64_t targets2x = shift(pushes & pawn_rank, push[ahead]) & legal_sq & ~pos.side[both];
	while (targets2x)
	{
		uint64_t target = 1ULL << lsb(targets2x);
		const auto to_sq = lsb(targets2x);
		const auto from_sq = to_sq - double_push[back];

		assert(1ULL << from_sq & pos.pieces[pawn] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, no_piece);

		targets2x &= targets2x - 1;
	}
}

void movegen::piece_moves(const board& pos, const gentype type)
{
	uint64_t targets;

	uint64_t pieces = pos.pieces[queen] & friends;
	while (pieces)
	{
		const auto sq = lsb(pieces);
		targets = slider_attacks(bishop_slider, sq, pos.side[both]) | slider_attacks(rook_slider, sq, pos.side[both]);
		targets &= gen_type[type] & legal_sq & pinned[sq];

		while (targets)
		{
			movelist[move_cnt++] = encode(sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[knight] & friends;
	while (pieces)
	{
		const auto sq = lsb(pieces);

		targets = knight_table[sq] & gen_type[type] & legal_sq & pinned[sq];
		while (targets)
		{
			movelist[move_cnt++] = encode(sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[bishop] & friends;
	while (pieces)
	{
		const auto sq = lsb(pieces);

		targets = slider_attacks(bishop_slider, sq, pos.side[both]) & gen_type[type] & legal_sq & pinned[sq];
		while (targets)
		{
			movelist[move_cnt++] = encode(sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[rook] & friends;
	while (pieces)
	{
		const auto sq = lsb(pieces);

		targets = slider_attacks(rook_slider, sq, pos.side[both]) & gen_type[type] & legal_sq & pinned[sq];
		while (targets)
		{
			movelist[move_cnt++] = encode(sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}
}

void movegen::pin(const board& pos)
{
	pin_cnt = 0;
	const auto king_sq = pos.king_sq[pos.turn];

	uint64_t att = slide_ray[rook_slider][king_sq] & enemies & (pos.pieces[rook] | pos.pieces[queen]);
	att |= slide_ray[bishop_slider][king_sq] & enemies & (pos.pieces[bishop] | pos.pieces[queen]);

	while (att)
	{
		uint64_t ray_to_att = slider_attacks(rook_slider, king_sq, att);
		ray_to_att |= slider_attacks(bishop_slider, king_sq, att);
		const uint64_t attacker = 1ULL << lsb(att);

		if (!(attacker & ray_to_att))
		{
			att &= att - 1;
			continue;
		}

		assert(fr_king);
		uint64_t x_ray = 0;
		for (const auto dir : ray)
		{
			auto flood = fr_king;
			for (; !(flood & dir.border); flood |= shift(flood, dir.shift));

			if (flood & attacker)
			{
				x_ray = flood & ray_to_att;
				break;
			}
		}

		assert(x_ray & attacker);
		assert(!(x_ray & fr_king));

		if (x_ray & friends && popcnt(x_ray & pos.side[both]) == 2)
		{
			assert(popcnt(x_ray & friends) == 1);
			pinned[lsb(x_ray & friends)] = x_ray;
			pin_idx[pin_cnt++] = lsb(x_ray & friends);
		}

		else if (pos.ep_square
			&& x_ray & friends & pos.pieces[pawn]
			&& x_ray & enemies & pos.pieces[pawn]
			&& popcnt(x_ray & pos.side[both]) == 3)
		{
			assert(popcnt(x_ray & enemies) == 2);

			const uint64_t enemy_pawn = x_ray & enemies & pos.pieces[pawn];

			if (const uint64_t friend_pawn = x_ray & friends & pos.pieces[pawn]; friend_pawn << 1 == enemy_pawn || friend_pawn >> 1 == enemy_pawn)
			{
				if (pos.ep_square == shift(enemy_pawn, push[pos.turn]))
				{
					pinned[lsb(x_ray & friends)] = ~pos.ep_square;
					pin_idx[pin_cnt++] = lsb(x_ray & friends);
				}
			}
		}

		att &= att - 1;
	}
}

uint64_t movegen::slider_attacks(const int sl, const int sq, uint64_t occ)
{
	assert(sq < 64 && sq >= 0);
	assert(sl == rook_slider || sl == bishop_slider);

	occ &= slider[sl][sq].mask;
	occ *= slider[sl][sq].magic;
	occ >>= slider[sl][sq].shift;
	return attack_table[slider[sl][sq].offset + occ];
}

void movegen::unpin()
{
	if (pin_cnt != 0)
	{
		assert(pin_cnt <= 8);
		for (int i = 0; i < pin_cnt; ++i)
			pinned[pin_idx[i]] = 0xffffffffffffffff;
	}
}
