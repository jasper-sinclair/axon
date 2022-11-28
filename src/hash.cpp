#include <cassert>
#include "bitops.h"
#include "hash.h"

#include "eval.h"
#include "random.h"

void hash::clear()
{
	erase();
	table_ = new trans[hash_size];
}

int hash::create(uint64_t size)
{
	erase();
	if (size > max_hash)
		size = max_hash;
	auto size_temp = (size << 20) / sizeof(trans) >> 1;

	hash_size = 1ULL;
	for (; hash_size <= size_temp; hash_size <<= 1);

	table_ = new trans[hash_size];
	size_temp = hash_size * sizeof(trans) >> 20;

	assert(size_temp <= size && size_temp <= max_hash);
	return static_cast<int>(size_temp);
}

void hash::erase()
{
	if (table_ != nullptr)
	{
		delete[] table_;
		table_ = nullptr;
	}
}

uint64_t to_key(const board& pos)
{
	uint64_t key = 0;

	for (int col = white; col <= black; ++col)
	{
		uint64_t pieces = pos.side[col];
		while (pieces)
		{
			const auto sq_old = lsb(pieces);
			assert(pos.piece_sq[sq_old] != no_piece);
			key ^= random_u64_number();
			pieces &= pieces - 1;
		}
	}

	for (const unsigned char i : castleright)
	{
		if (pos.castle_rights & i)
			key ^= random_u64_number();
	}

	if (pos.ep_square)
	{
		const auto file_idx = lsb(pos.ep_square) & 7;
		assert((lsb(pos.ep_square) - file_idx) % 8 == 0);

		if (pos.pieces[pawn] & pos.side[pos.turn ^ 1] & ep_flank[pos.turn][file_idx])
			key ^= random_u64_number();
	}

	key ^= is_turn[pos.turn];

	return key;
}

bool hash::probe(const board& pos, uint16_t& move, int& score, const int ply, const int depth, uint8_t& flag)
{
	if (const trans* entry = &table_[pos.key & hash_size - 1]; entry->key == pos.key)
	{
		move = entry->move;
		flag = entry->flag;

		if (entry->ply >= ply)
		{
			score = entry->score;

			if (score > max_score) score -= depth;
			if (score < -max_score) score += depth;

			assert(abs(score) <= scoretype::mate_score);
			return true;
		}
		return false;
	}

	return false;
}

void hash::store(const board& pos, const uint16_t move, int score, const int ply, const int depth, const uint8_t flag)
{
	if (score == draw_score)
		return;

	trans* entry = &table_[pos.key & hash_size - 1];

	if (entry->key == pos.key && entry->ply > ply)
		return;

	if (score > max_score) score += depth;
	if (score < -max_score) score -= depth;
	assert(abs(score) <= scoretype::mate_score);

	entry->key = pos.key;
	entry->move = move;
	entry->score = static_cast<int16_t>(score);
	entry->ply = static_cast<int8_t>(ply);
	entry->flag = flag;
}
