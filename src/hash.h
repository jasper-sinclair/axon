#pragma once
#include "board.h"
#include "common.h"

namespace hash
{
	struct tt_entry
	{
		uint64_t key;
		uint16_t move;
		int16_t score;
		uint8_t ply;
		uint8_t flag;
	};

	const struct offsets
	{
		const int castl{768};
		const int ep{772};
	}
	offset;

	constexpr int piece_12[]{ 1, 7, 3, 5, 9, 11 };
	constexpr uint64_t is_turn[]{ 0xF8D626AAAF278509ULL,0x0ULL };
	constexpr uint64_t ep_flank[][8]
	{
		{ 0x0002000000, 0x0005000000, 0x000a000000, 0x0014000000,
			0x0028000000, 0x0050000000, 0x00a0000000, 0x0040000000 },
		{ 0x0200000000, 0x0500000000, 0x0a00000000, 0x1400000000,
			0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000 }
	};

	inline tt_entry* tt { nullptr };
	inline uint64_t tt_size { 0 };

	uint64_t to_key(const board& pos);

	void tt_clear();
	void tt_delete();
	int tt_create(uint64_t size);
	void tt_save(const board& pos, uint16_t move, int score, int ply, uint8_t flag);
	bool tt_probe(int ply, const board& pos, int& score, uint16_t& move, uint8_t& flag);
}
