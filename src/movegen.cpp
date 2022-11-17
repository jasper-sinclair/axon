#include <algorithm>
#include <cassert>
#include "bitops.h"
#include "movegen.h"
#include "magic.h"

void movegen::init(){for (auto& p : pinned) p = 0xffffffffffffffff;}
bool movegen::in_list(const uint16_t move) { return find(move) != movelist + move_cnt; }
uint16_t* movegen::find(const uint16_t move) { return std::find(movelist, movelist + move_cnt, move); }

void movegen::legal(const board& pos)
{
	const uint64_t side[]{ ~(king_fr - 1), king_fr - 1 };
	assert(king_fr != 0);

	uint64_t attackers{ slider_attacks(rook,static_cast<int>(sq_king_fr), pos.side[both]) & (pos.pieces[rooks] | pos.pieces[queens]) };
	attackers |= slider_attacks(bishop, static_cast<int>(sq_king_fr), pos.side[both]) & (pos.pieces[bishops] | pos.pieces[queens]);
	attackers |= knight_table[sq_king_fr] & pos.pieces[knights];
	attackers |= king_table[sq_king_fr] & pos.pieces[pawns] & slide_ray[bishop][sq_king_fr] & side[pos.turn];
	attackers &= enemies;

	if (const int nr_att{ popcnt(attackers) }; nr_att == 0)
		legal_sq = 0xffffffffffffffff;
	else if (nr_att == 1)
	{
		if (attackers & pos.pieces[knights] || attackers & pos.pieces[pawns])
			legal_sq = attackers;
		else
		{
			assert(attackers & pos.pieces[rooks] || attackers & pos.pieces[bishops] || attackers & pos.pieces[queens]);

			uint64_t single_ray[8]{ 0 };
			for (int dir{ 0 }; dir < 8; ++dir)
			{
				uint64_t flood{ king_fr };
				while (!(flood & ray[dir].border))
				{
					shift_real(flood, ray[dir].shift);
					single_ray[dir] |= flood;
				}
			}

			for (const auto& s : single_ray)
				if (s & attackers)
				{
					legal_sq = s & (slider_attacks(rook, static_cast<int>(sq_king_fr), pos.side[both])
						| slider_attacks(bishop, static_cast<int>(sq_king_fr), pos.side[both]));
					break;
				}
		}
	}
	else
	{
		assert(nr_att == 2);
		legal_sq = 0ULL;
	}
}

void movegen::pin(const board& pos) const
{
	uint64_t single_ray[8]{ 0 };

	uint64_t attackers{ slide_ray[rook][sq_king_fr] & enemies & (pos.pieces[rooks] | pos.pieces[queens]) };
	attackers |= slide_ray[bishop][sq_king_fr] & enemies & (pos.pieces[bishops] | pos.pieces[queens]);

	while (attackers)
	{
		uint64_t rays_att{ slider_attacks(rook, static_cast<int>(sq_king_fr), attackers) };
		rays_att |= slider_attacks(bishop, static_cast<int>(sq_king_fr), attackers);

		uint64_t pinning_ray{ 0 };
		const uint64_t attacker{ 1ULL << lsb(attackers) };

		if (!(attacker & rays_att))
		{
			attackers &= attackers - 1;
			continue;
		}

		assert(king_fr != 0);

		if (std::all_of(single_ray, single_ray + 8, [](const uint64_t i) {return i == 0ULL; }))
		{
			for (int dir{ 0 }; dir < 8; ++dir)
			{
				uint64_t flood{ king_fr };
				while (!(flood & ray[dir].border))
				{
					shift_real(flood, ray[dir].shift);
					single_ray[dir] |= flood;
				}
			}
		}

		for (const auto& s : single_ray)
			if (s & attacker)
			{
				pinning_ray = s & rays_att;
				break;
			}

		assert(pinning_ray & attacker);

		if (pinning_ray & friends && popcnt(pinning_ray & pos.side[both]) == 2)
		{
			assert(popcnt(pinning_ray & friends) == 1);

			pinned[lsb(pinning_ray & friends)] = pinning_ray;
			pin_idx[pin_cnt++] = lsb(pinning_ray & friends);
		}

		else if (pos.ep_square
			&& pinning_ray & friends & pos.pieces[pawns]
			&& pinning_ray & enemies & pos.pieces[pawns]
			&& popcnt(pinning_ray & pos.side[both]) == 3)
		{
			assert(popcnt(pinning_ray & enemies) == 2);
			uint64_t enemy_pawn{ pinning_ray & enemies & pos.pieces[pawns] };

			if ((pinning_ray & friends & pos.pieces[pawns]) << 1 == (pinning_ray & enemies & pos.pieces[pawns])
				|| (pinning_ray & friends & pos.pieces[pawns]) >> 1 == (pinning_ray & enemies & pos.pieces[pawns]))
			{
				assert(popcnt(pinning_ray & enemies & pos.pieces[pawns]) == 1);

				shift_real(enemy_pawn, push[pos.turn]);

				if (pos.ep_square == enemy_pawn)
				{
					pinned[lsb(pinning_ray & friends)] = ~enemy_pawn;
					pin_idx[pin_cnt++] = lsb(pinning_ray & friends);
				}
			}
		}
		attackers &= attackers - 1;
	}
}

int movegen::gen_moves(const board& pos, const mvgen type)
{
	ahead = static_cast<uint8_t>(pos.turn);
	back = static_cast<uint8_t>(pos.turn * 2);

	friends = pos.side[pos.turn];
	enemies = pos.side[pos.turn ^ 1];
	pawn_rank = sec_rank[pos.turn];

	gentype[captures] = enemies;
	gentype[quiets] = ~pos.side[both];

	border_right = bfile[pos.turn ^ 1];
	border_left = bfile[pos.turn];

	king_fr = friends & pos.pieces[kings];
	sq_king_fr = lsb(king_fr);

	move_cnt = 0;
	pin_cnt = 0;

	legal(pos);
	pin(pos);

	king_moves(pos, captures);
	pawn_promotions(pos);
	pawn_captures(pos);
	piece_moves(pos, captures);

	if (type == all)
	{
		pawn_quiet_moves(pos);
		piece_moves(pos, quiets);
		king_moves(pos, quiets);
	}

	unpin();
	return move_cnt;
}

void movegen::pawn_promotions(const board& pos)
{
	uint64_t targets{ shift(pos.pieces[pawns] & friends, push[ahead]) & ~pos.side[both] & legal_sq & promo_rank };
	while (targets)
	{
		uint64_t target{ 1ULL << lsb(targets) };
		const auto to_sq{ lsb(targets) };
		const auto from_sq{ to_sq - push[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag{ 15 }; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}

	targets = shift(pos.pieces[pawns] & friends & border_left, cap_left[ahead]) & legal_sq & promo_rank & enemies;
	while (targets)
	{
		uint64_t target{ 1ULL << lsb(targets) };
		const auto to_sq{ lsb(targets) };
		const auto from_sq{ to_sq - cap_left[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag{ 15 }; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}

	targets = shift(pos.pieces[pawns] & friends & border_right, cap_right[ahead]) & legal_sq & promo_rank & enemies;
	while (targets)
	{
		uint64_t target{ 1ULL << lsb(targets) };
		const auto to_sq{ lsb(targets) };
		const auto from_sq{ to_sq - cap_right[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
		{
			for (int flag{ 15 }; flag >= 12; --flag)
				movelist[move_cnt++] = encode(from_sq, to_sq, flag);
		}

		targets &= targets - 1;
	}
}

void movegen::pawn_captures(const board& pos)
{
	uint64_t targets{ shift(pos.pieces[pawns] & friends & border_left, cap_left[ahead]) & ~promo_rank };
	uint64_t targets_cap{ targets & enemies & legal_sq };

	while (targets_cap)
	{
		uint64_t target{ 1ULL << lsb(targets_cap) };
		const auto to_sq{ lsb(targets_cap) };
		const auto from_sq{ to_sq - cap_left[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, pos.piece_sq[to_sq]);

		targets_cap &= targets_cap - 1;
	}

	uint64_t target_ep{ targets & pos.ep_square & shift(legal_sq, push[ahead]) };
	if (target_ep)
	{
		const auto to_sq{ lsb(target_ep) };
		const auto from_sq{ to_sq - cap_left[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target_ep &= pinned[from_sq];
		if (target_ep)
			movelist[move_cnt++] = encode(from_sq, to_sq, enpassant);
	}

	targets = shift(pos.pieces[pawns] & friends & border_right, cap_right[ahead]) & ~promo_rank;
	targets_cap = targets & enemies & legal_sq;

	while (targets_cap)
	{
		uint64_t target{ 1ULL << lsb(targets_cap) };
		const auto to_sq{ lsb(targets_cap) };
		const auto from_sq{ to_sq - cap_right[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, pos.piece_sq[to_sq]);

		targets_cap &= targets_cap - 1;
	}

	target_ep = targets & pos.ep_square & shift(legal_sq, push[ahead]);
	if (target_ep)
	{
		const auto to_sq{ lsb(target_ep) };
		const auto from_sq{ to_sq - cap_right[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target_ep &= pinned[from_sq];
		if (target_ep)
			movelist[move_cnt++] = encode(from_sq, to_sq, enpassant);
	}
}

void movegen::pawn_quiet_moves(const board& pos)
{
	const uint64_t pushes{ shift(pos.pieces[pawns] & friends, push[ahead]) & ~pos.side[both] & ~promo_rank };
	uint64_t targets{ pushes & legal_sq };

	while (targets)
	{
		uint64_t target{ 1ULL << lsb(targets) };
		const auto to_sq{ lsb(targets) };
		const auto from_sq{ to_sq - push[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, no_piece);

		targets &= targets - 1;
	}

	uint64_t targets2x{ shift(pushes & pawn_rank, push[ahead]) & legal_sq & ~pos.side[both] };
	while (targets2x)
	{
		uint64_t target{ 1ULL << lsb(targets2x) };
		const auto to_sq{ lsb(targets2x) };
		const auto from_sq{ to_sq - double_push[back] };

		assert((1ULL << from_sq) & pos.pieces[pawns] & friends);

		target &= pinned[from_sq];
		if (target)
			movelist[move_cnt++] = encode(from_sq, to_sq, no_piece);

		targets2x &= targets2x - 1;
	}
}

void movegen::king_moves(const board& pos, const mvgen type)
{
	uint64_t targets{ check(pos, pos.turn, king_table[sq_king_fr] & gentype[type]) };
	while (targets)
	{
		movelist[move_cnt++] = encode(sq_king_fr, lsb(targets), pos.piece_sq[lsb(targets)]);
		targets &= targets - 1;
	}

	if (type == quiets && king_fr & 0x800000000000008)
	{
		constexpr uint64_t r[]{ 0xff, 0xff00000000000000 };
		const uint64_t rank_king{ r[pos.turn] };

		constexpr uint8_t rights_s[]{ 0x1, 0x10 };
		constexpr uint8_t rights_l[]{ 0x4, 0x40 };

		if (rights_s[pos.turn] & pos.castle_rights
			&& !(pos.side[both] & 0x600000000000006 & rank_king)
			&& popcnt(check(pos, pos.turn, 0x0e0000000000000e & rank_king)) == 3)
		{
			constexpr uint32_t target[]{ 1, 57 };
			movelist[move_cnt++] = encode(sq_king_fr, target[pos.turn], white_kingside + pos.turn * 2);
		}
		if (rights_l[pos.turn] & pos.castle_rights
			&& !(pos.side[both] & 0x7000000000000070 & rank_king)
			&& popcnt(check(pos, pos.turn, 0x3800000000000038 & rank_king)) == 3)
		{
			constexpr uint32_t target[]{ 5, 61 };
			movelist[move_cnt++] = encode(sq_king_fr, target[pos.turn], white_queenside + pos.turn * 2);
		}
	}
}

void movegen::piece_moves(const board& pos, const mvgen type)
{
	uint64_t targets;

	uint64_t pieces{ pos.pieces[queens] & friends };
	while (pieces)
	{
		const auto queen{ lsb(pieces) };
		targets = slider_attacks(bishop, queen, pos.side[both]) | slider_attacks(rook, queen, pos.side[both]);
		targets &= gentype[type] & legal_sq & pinned[queen];

		while (targets)
		{
			movelist[move_cnt++] = encode(queen, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[knights] & friends;
	while (pieces)
	{
		const auto knight{ lsb(pieces) };
		targets = knight_table[knight] & gentype[type] & legal_sq & pinned[knight];

		while (targets)
		{
			movelist[move_cnt++] = encode(knight, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[bishops] & friends;
	while (pieces)
	{
		const auto bishop_sq{ lsb(pieces) };
		targets = slider_attacks(bishop, bishop_sq, pos.side[both]) & gentype[type] & legal_sq & pinned[bishop_sq];

		while (targets)
		{
			movelist[move_cnt++] = encode(bishop_sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}

	pieces = pos.pieces[rooks] & friends;
	while (pieces)
	{
		const auto rook_sq{ lsb(pieces) };

		targets = slider_attacks(rook, rook_sq, pos.side[both]) & gentype[type] & legal_sq & pinned[rook_sq];

		while (targets)
		{
			movelist[move_cnt++] = encode(rook_sq, lsb(targets), pos.piece_sq[lsb(targets)]);
			targets &= targets - 1;
		}
		pieces &= pieces - 1;
	}
}

uint64_t movegen::check(const board& pos, const int turn, uint64_t squares)
{
	assert(turn == white || turn == black);

	const uint64_t king{ pos.side[turn] & pos.pieces[kings] };
	const uint64_t enemy{ pos.side[turn ^ 1] };
	uint64_t inquire{ squares };

	while (inquire)
	{
		const auto sq{ lsb(inquire) };
		const uint64_t sq64{ 1ULL << sq };
		const uint64_t side[]{ ~(sq64 - 1), sq64 - 1 };

		uint64_t attacker{ slider_attacks(rook, sq, pos.side[both] & ~king) & (pos.pieces[rooks] | pos.pieces[queens]) };
		attacker |= slider_attacks(bishop, sq, pos.side[both] & ~king) & (pos.pieces[bishops] | pos.pieces[queens]);
		attacker |= knight_table[sq] & pos.pieces[knights];
		attacker |= king_table[sq] & pos.pieces[kings];
		attacker |= king_table[sq] & pos.pieces[pawns] & slide_ray[bishop][sq] & side[turn];
		attacker &= enemy;

		if (attacker)
			squares ^= sq64;
		inquire &= inquire - 1;
	}
	return squares;
}

uint64_t attack::by_pawns(const board& pos, const int col)
{
	assert(col == white || col == black);

	uint64_t att_table{ shift(pos.pieces[pawns] & pos.side[col] & ~border[col], cap_left[col]) };
	att_table |= shift(pos.pieces[pawns] & pos.side[col] & ~border[col ^ 1], cap_right[col]);

	return att_table;
}

uint64_t movegen::slider_attacks(const uint8_t sl, const int sq, uint64_t occ)
{
	assert(sq < 64 && sq >= 0);
	assert(sl == rook || sl == bishop);

	occ &= magic::slider[sl][sq].mask;
	occ *= magic::slider[sl][sq].magic;
	occ >>= magic::slider[sl][sq].shift;
	return attack_table[magic::slider[sl][sq].offset + occ];
}

void movegen::unpin()
{
	if (pin_cnt != 0)
	{
		assert(pin_cnt <= 8);
		for (int i{ 0 }; i < pin_cnt; ++i)
			pinned[pin_idx[i]] = 0xffffffffffffffff;
	}
}

