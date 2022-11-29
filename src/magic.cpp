#include <cassert>
#include <random>
#include "bitops.h"
#include "board.h"
#include "common.h"
#include "magic.h"
#include "movegen.h"
#include "random.h"

uint64_t attack::by_pawns(const board& pos, const int col)
{
	assert(col == white || col == black);

	uint64_t att_table = shift(pos.pieces[pawn] & pos.side[col] & ~border[col], cap_left[col]);
	att_table |= shift(pos.pieces[pawn] & pos.side[col] & ~border[col ^ 1], cap_right[col]);

	return att_table;
}

void magic::init_blocker(const int sl, std::vector<uint64_t>& blocker)
{
	assert(sl == rook_slider || sl == bishop_slider);
	assert(blocker.empty() || blocker.size() == 102400U);

	bool bit[12]{false};

	for (int sq = 0; sq < 64; ++sq)
	{
		slider[sl][sq].offset = blocker.size();

		uint64_t mask_split[12]{0};
		int bits_in = 0;

		uint64_t mask_bit = slider[sl][sq].mask;
		while (mask_bit)
		{
			mask_split[bits_in++] = 1ULL << lsb(mask_bit);
			mask_bit &= mask_bit - 1;
		}
		assert(bits_in <= 12);
		assert(bits_in >= 5);
		assert(popcnt(slider[sl][sq].mask) == bits_in);

		slider[sl][sq].shift = 64 - bits_in;

		const int max = 1 << bits_in;
		for (int a = 0; a < max; ++a)
		{
			uint64_t pos = 0;
			for (int b = 0; b < bits_in; ++b)
			{
				if (!(a % (1U << b)))
					bit[b] = !bit[b];
				if (bit[b])
					pos |= mask_split[b];
			}
			blocker.push_back(pos);
		}
	}
}

void magic::init_connect(const int sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp)
{
	assert(sl == rook_slider || sl == bishop_slider);
	assert(attack_table.empty() || attack_table.size() == 102400U);

	for (int sq = 0; sq < 64; ++sq)
	{
		const int max = 1 << (64 - slider[sl][sq].shift);

		for (int cnt = 0; cnt < max; ++cnt)
		{
			attack_table[slider[sl][sq].offset + (blocker[slider[sl][sq].offset + cnt] * slider[sl][sq].magic >> slider[sl][sq].shift)]
				= attack_temp[slider[sl][sq].offset + cnt];
		}
	}
}

void magic::init_king()
{
	for (int sq = 0; sq < 64; ++sq)
	{
		const uint64_t sq64 = 1ULL << sq;

		for (const auto dir : ray)
		{
			if (const uint64_t att = sq64; !(att & dir.border))
				movegen::king_table[sq] |= shift(att, dir.shift);
		}
	}
}

void magic::init_knight()
{
	constexpr pattern jump[]
	{
		{15, 0xffff010101010101}, {6, 0xff03030303030303},
		{54, 0x03030303030303ff}, {47, 0x010101010101ffff},
		{49, 0x808080808080ffff}, {58, 0xc0c0c0c0c0c0c0ff},
		{10, 0xffc0c0c0c0c0c0c0}, {17, 0xffff808080808080}
	};

	for (int sq = 0; sq < 64; ++sq)
	{
		const uint64_t sq64 = 1ULL << sq;

		for (const auto dir : jump)
		{
			if (const uint64_t att = sq64; !(att & dir.border))
				movegen::knight_table[sq] |= shift(att, dir.shift);
		}
	}
}

void magic::init_magic()
{
	attack_table.clear();
	attack_table.reserve(table_size);

	std::vector<uint64_t> attack_temp;
	attack_temp.reserve(table_size);

	std::vector<uint64_t> blocker;
	blocker.reserve(table_size);

	for (int sl = rook_slider; sl <= bishop_slider; ++sl)
	{
		init_mask(sl);
		init_blocker(sl, blocker);
		init_move(sl, blocker, attack_temp);
		init_magic(sl, blocker, attack_temp);
		init_connect(sl, blocker, attack_temp);
	}
}

void magic::init_magic(const int sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp)
{
	bool fail;
	for (int sq = 0; sq < 64; ++sq)
	{
		std::vector<uint64_t> occ;

		const int occ_size = 1 << (64 - slider[sl][sq].shift);
		occ.resize(occ_size);

		assert(occ.size() <= 4096U);
		assert(sq / 4 <= 16 && sq / 4 >= 0);

		rand_xor rand_gen{seeds[sq / 4]};

		const int max = 1 << (64 - slider[sl][sq].shift);

		do
		{
			do
			{
				slider[sl][sq].magic = rand_gen.sparse64();
			}
			while (popcnt(slider[sl][sq].mask * slider[sl][sq].magic & 0xff00000000000000) < 6);

			fail = false;
			occ.clear();
			occ.resize(occ_size);

			for (int i = 0; !fail && i < max; ++i)
			{
				const auto idx = blocker[slider[sl][sq].offset + i] * slider[sl][sq].magic >> slider[sl][sq].shift;
				assert(idx <= static_cast<uint64_t>(occ_size));

				if (!occ[idx])
					occ[idx] = attack_temp[slider[sl][sq].offset + i];
				else if (occ[idx] != attack_temp[slider[sl][sq].offset + i])
				{
					fail = true;
					break;
				}
			}
		}
		while (fail);
	}
}

void magic::init_mask(const int sl)
{
	assert(sl == rook_slider || sl == bishop_slider);

	for (int sq = 0; sq < 64; ++sq)
	{
		const uint64_t sq64 = 1ULL << sq;

		for (int dir = sl; dir < 8; dir += 2)
		{
			uint64_t flood = sq64;
			while (!(flood & ray[dir].border))
			{
				slider[sl][sq].mask |= flood;
				real_shift(flood, ray[dir].shift);
			}
		}
		slider[sl][sq].mask ^= sq64;
	}
}

void magic::init_move(const int sl, const std::vector<uint64_t>& blocker, std::vector<uint64_t>& attack_temp)
{
	assert(sl == rook_slider || sl == bishop_slider);
	assert(attack_temp.empty() || attack_temp.size() == 102400U);

	for (int sq = 0; sq < 64; ++sq)
	{
		const uint64_t sq64 = 1ULL << sq;

		const int max = 1 << (64 - slider[sl][sq].shift);
		for (int cnt = 0; cnt < max; ++cnt)
		{
			uint64_t pos = 0;

			for (int dir = sl; dir < 8; dir += 2)
			{
				uint64_t flood = sq64;
				while (!(flood & ray[dir].border) && !(flood & blocker[slider[sl][sq].offset + cnt]))
				{
					real_shift(flood, ray[dir].shift);
					pos |= flood;
				}
			}
			attack_temp.push_back(pos);

			assert(attack_temp.size() - 1 == slider[sl][sq].offset + cnt);
		}
	}
}

void magic::init_ray(const int sl)
{
	assert(sl == rook_slider || sl == bishop_slider);

	for (int sq = 0; sq < 64; ++sq)
	{
		const uint64_t sq64 = 1ULL << sq;

		for (int dir = sl; dir < 8; dir += 2)
		{
			uint64_t flood = sq64;
			while (!(flood & ray[dir].border))
			{
				real_shift(flood, ray[dir].shift);
				slide_ray[sl][sq] |= flood;
			}
		}
	}
}

void magic::init_seeds()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(100000, 999999);

	for (int i = 0; i <= 15; ++i)
		seeds[i] = dist(rng);
}
