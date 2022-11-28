#pragma once
#include "board.h"
#include "common.h"
#include "random.h"

class hash
{
	struct trans
	{
		int16_t score;
		uint16_t move;
		uint64_t key;
		uint8_t flag;
		uint8_t ply;
	};

	static trans* table_;
	static void erase();

public:
	hash() = default;

	explicit hash(const uint64_t size)
	{
		erase();
		create(size);
	}

	~hash()
	{
		erase();
	}

	static uint64_t hash_size;
	static int create(uint64_t size);
	static void clear();
	static void store(const board& pos, uint16_t move, int score, int ply, int depth, uint8_t flag);
	static bool probe(const board& pos, uint16_t& move, int& score, int ply, int depth, uint8_t& flag);
};

const struct offset_s
{
	const int castl = 768;
	const int ep = 772;
	const int turn = 780;
}
offset;

const uint64_t is_turn[]
{
	random_u64_number(),
	0ULL
};

constexpr uint64_t ep_flank[][8]
{
	{
		0x0002000000, 0x0005000000, 0x000a000000, 0x0014000000,
		0x0028000000, 0x0050000000, 0x00a0000000, 0x0040000000
	},
	{
		0x0200000000, 0x0500000000, 0x0a00000000, 0x1400000000,
		0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000
	}
};

constexpr int piece_12[]{1, 7, 3, 5, 9, 11};
inline hash::trans* hash::table_ = nullptr;
inline uint64_t hash::hash_size = 0;
uint64_t to_key(const board& pos);
