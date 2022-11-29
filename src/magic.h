#pragma once
#include "common.h"
#include "movegen.h"

namespace magic
{
	struct entry
	{
		int shift;
		size_t offset;
		uint64_t magic;
		uint64_t mask;
	};

	struct pattern
	{
		int shift;
		uint64_t border;
	};

	inline uint64_t seeds[16];
	void init_blocker(int sl, std::vector<uint64_t>& blocker);
	void init_connect(int sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp);
	void init_king();
	void init_knight();
	void init_magic();
	void init_magic(int sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp);
	void init_mask(int sl);
	void init_move(int sl, const std::vector<uint64_t>& blocker, std::vector<uint64_t>& attack_temp);
	void init_ray(int sl);
	void init_seeds();
}

namespace attack
{
	uint64_t by_pawns(const board& pos, int col);
}

inline void real_shift(uint64_t& bb, const int shift)
{
	bb = bb << shift | bb >> (64 - shift);
}

constexpr magic::pattern ray[]
{
	{8, 0xff00000000000000}, {7, 0xff01010101010101},
	{63, 0x0101010101010101}, {55, 0x01010101010101ff},
	{56, 0x00000000000000ff}, {57, 0x80808080808080ff},
	{1, 0x8080808080808080}, {9, 0xff80808080808080}
};

constexpr int cap_left[]{9, 55, -9};
constexpr int cap_right[]{7, 57, -7};
constexpr int double_push[]{16, 48, -16};
constexpr int push[]{8, 56, -8};
constexpr int table_size = 107648;

constexpr uint64_t border[]{file[fa], file[fh]};
constexpr uint64_t promo_rank = rank[r1] | rank[r8];
constexpr uint64_t third_rank[]{rank[r3], rank[r6]};

inline int pin_cnt;
inline magic::entry slider[2][64];
inline std::vector<uint64_t> attack_table;

inline uint32_t pin_idx[8];
inline uint64_t border_left;
inline uint64_t border_right;
inline uint64_t enemies;
inline uint64_t fr_king;
inline uint64_t friends;
inline uint64_t gen_type[2];
inline uint64_t pawn_rank;
inline uint64_t slide_ray[2][64];
inline uint8_t ahead;
inline uint8_t back;

