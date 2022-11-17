#include <cassert>
#include "bitops.h"
#include "hash.h"
#include "random.h"

uint64_t hash::to_key(const board& pos)
{
	uint64_t key{0};

	for (int col{white}; col <= black; ++col)
	{
		uint64_t pieces{pos.side[col]};
		while (pieces)
		{
			key ^= random_u64_number();
			pieces &= pieces - 1;
		}
	}

	for (int i{0}; i < 4; ++i)
	{
		if (pos.castle_rights & castleright[i])
			key ^= random_u64_number();
	}

	if (pos.ep_square)
	{
		const auto file{ lsb(pos.ep_square) & 7};
		assert((lsb(pos.ep_square) - file) % 8 == 0);

		if (pos.pieces[pawns] & pos.side[pos.turn ^ 1] & ep_flank[pos.turn][file])
			key ^= random_u64_number();
	}

	key ^= is_turn[pos.turn];

	return key;
}

int hash::tt_create(uint64_t size)
{
	assert(tt == nullptr);
	if (size > max_hash)
		size = max_hash;
	auto size_temp{(size << 20) / sizeof(tt_entry) >> 1};

	tt_size = 1ULL;
	for (; tt_size <= size_temp; tt_size <<= 1);

	tt = new tt_entry[tt_size];
	size_temp = tt_size * sizeof(tt_entry) >> 20;

	assert(size_temp <= size && size_temp <= max_hash);
	return static_cast<int>(size_temp);
}

void hash::tt_clear()
{
	tt_delete();
	tt = new tt_entry[tt_size];
}

void hash::tt_delete()
{
	if (tt != nullptr)
	{
		delete[] tt;
		tt = nullptr;
	}
}

void hash::tt_save(const board& pos, const uint16_t move, const int score, const int ply, const uint8_t flag)
{
	tt_entry* new_entry { &tt[pos.key & tt_size - 1] };

	if (new_entry->key == pos.key && new_entry->ply > ply) return;

	new_entry->key = pos.key;
	new_entry->move = move;
	new_entry->score = static_cast<int16_t>(score);
	new_entry->ply = static_cast<uint8_t>(ply);
	new_entry->flag = flag;
}

bool hash::tt_probe(const int ply, const board& pos, int& score, uint16_t& move, uint8_t& flag)
{
	if (const tt_entry* entry { &tt[pos.key & tt_size - 1] }; entry->key == pos.key)
	{
		move = entry->move;
		flag = entry->flag;

		if (entry->ply >= ply)
		{
			score = entry->score;
			return true;
		}
		return false;
	}

	return false;
}
