#include <algorithm>
#include <cassert>
#include <random>
#include "bitops.h"
#include "magic.h"
#include "movegen.h"
#include "random.h"

void magic::init_magic()
{
	attack_table.clear();
	attack_table.reserve(table_size);

	std::vector<uint64_t> attack_temp;
	attack_temp.reserve(table_size);

	std::vector<uint64_t> blocker;
	blocker.reserve(table_size);

	for (uint8_t sl{rook}; sl <= bishop; ++sl)
	{
		init_mask(sl);
		init_blocker(sl, blocker);
		init_move(sl, blocker, attack_temp);
		init_magic(sl, blocker, attack_temp);
		init_connect(sl, blocker, attack_temp);
	}
}

void magic::init_mask(const uint8_t sl)
{
	assert(sl == rook || sl == bishop);

	for (int i{0}; i < 64; ++i)
	{
		const uint64_t sq{1ULL << i};
		assert(slider[sl][sq].mask == 0ULL);

		for (int dir{sl}; dir < 8; dir += 2)
		{
			uint64_t flood{sq};
			while (!(flood & ray[dir].border))
			{
				slider[sl][i].mask |= flood;
				shift_real(flood, ray[dir].shift);
			}
		}
		slider[sl][i].mask ^= sq;
	}
}

void magic::init_blocker(const uint8_t sl, std::vector<uint64_t>& blocker)
{
	bool bit[12]{false};

	assert(blocker.size() == 0U || blocker.size() == 102400U);

	for (int sq{0}; sq < 64; ++sq)
	{
		assert(slider[sl][sq].offset == 0);

		slider[sl][sq].offset = blocker.size();

		uint64_t mask_split[12]{};
		int bits_in{0};

		uint64_t mask_bit{slider[sl][sq].mask};
		while (mask_bit)
		{
			mask_split[bits_in++] = 1ULL << lsb(mask_bit);
			mask_bit &= mask_bit - 1;
		}
		assert(bits_in <= 12);
		assert(bits_in >= 5);
		assert(popcnt(slider[sl][sq].mask) == bits_in);
		assert(slider[sl][sq].shift == 0);

		slider[sl][sq].shift = 64 - bits_in;

		const int max{1 << bits_in};
		for (int a{0}; a < max; ++a)
		{
			uint64_t pos{0};
			for (int b{0}; b < bits_in; ++b)
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

void magic::init_move(const uint8_t sl, const std::vector<uint64_t>& blocker, std::vector<uint64_t>& attack_temp)
{
	assert(sl == rook || sl == bishop);
	assert(attack_temp.size() == 0U || attack_temp.size() == 102400U);

	for (int i{0}; i < 64; ++i)
	{
		const uint64_t sq{1ULL << i};

		const int max{1 << (64 - slider[sl][i].shift)};
		for (int cnt{0}; cnt < max; ++cnt)
		{
			uint64_t pos{0};

			for (int dir{sl}; dir < 8; dir += 2)
			{
				uint64_t flood{sq};
				while (!(flood & ray[dir].border) && !(flood & blocker[slider[sl][i].offset + cnt]))
				{
					shift_real(flood, ray[dir].shift);
					pos |= flood;
				}
			}
			attack_temp.push_back(pos);

			assert(attack_temp.size() - 1 == slider[sl][sq].offset + cnt);
		}
	}
}

void magic::init_magic(const uint8_t sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp)
{
	bool fail;
	for (int sq{0}; sq < 64; ++sq)
	{
		std::vector<uint64_t> occ;

		const int size_occ{1 << (64 - slider[sl][sq].shift)};
		occ.resize(size_occ);
		assert(occ.size() <= 4096U);

		assert(static_cast<int>(sq / 4) <= 16);
		assert(static_cast<int>(sq / 4) >= 0);

		rand_xor rand_gen{seeds[(sq / 4)]};

		const int max{1 << (64 - slider[sl][sq].shift)};

		assert(slider[sl][sq].magic == 0ULL);
		do
		{
			do
			{
				slider[sl][sq].magic = rand_gen.sparse64();
			}
			while (popcnt(slider[sl][sq].mask * slider[sl][sq].magic & 0xff00000000000000) < 6);

			fail = false;
			occ.clear();
			occ.resize(size_occ);

			for (int i{0}; !fail && i < max; ++i)
			{
				if (const int index{static_cast<int>(blocker[slider[sl][sq].offset + i] * slider[sl][sq].magic >> slider[sl][sq].shift)}; !occ[index])
					occ[index] = attack_temp[slider[sl][sq].offset + i];
				else if (occ[index] != attack_temp[slider[sl][sq].offset + i])
				{
					fail = true;
					break;
				}
			}
		}
		while (fail);
	}
}

void magic::init_connect(const uint8_t sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp)
{
	assert(sl == rook || sl == bishop);
	assert(attack_table.size() == 0U || attack_table.size() == 102400U);

	for (int sq{0}; sq < 64; ++sq)
	{
		const int max{1 << (64 - slider[sl][sq].shift)};

		for (int cnt{0}; cnt < max; ++cnt)
			attack_table[slider[sl][sq].offset
				+ static_cast<int>(blocker[slider[sl][sq].offset + cnt] * slider[sl][sq].magic
					>> slider[sl][sq].shift)] = attack_temp[slider[sl][sq].offset + cnt];
	}
}

void magic::init_ray(const uint8_t sl)
{
	assert(sl == rook || sl == bishop);

	for (int i{0}; i < 64; ++i)
	{
		const uint64_t sq{1ULL << i};
		assert(slide_ray[sl][sq] == 0ULL);

		for (int dir{sl}; dir < 8; dir += 2)
		{
			uint64_t flood{sq};
			while (!(flood & ray[dir].border))
			{
				shift_real(flood, ray[dir].shift);
				slide_ray[sl][i] |= flood;
			}
		}
	}
}

void magic::init_king()
{
	for (int i{0}; i < 64; ++i)
	{
		const uint64_t sq{1ULL << i};
		assert(king_table[sq] == 0ULL);

		for (int dir{0}; dir < 8; ++dir)
		{
			if (const uint64_t att{sq}; !(att & ray[dir].border))
			{
				king_table[i] |= shift(att, ray[dir].shift);
			}
		}
	}
}

void magic::init_knight()
{
	constexpr rays pattern[]
	{
		{15, 0xffff010101010101}, {6, 0xff03030303030303},
		{54, 0x03030303030303ff}, {47, 0x010101010101ffff},
		{49, 0x808080808080ffff}, {58, 0xc0c0c0c0c0c0c0ff},
		{10, 0xffc0c0c0c0c0c0c0}, {17, 0xffff808080808080}
	};

	for (int i{0}; i < 64; ++i)
	{
		const uint64_t sq{1ULL << i};
		assert(knight_table[sq] == 0ULL);

		for (int dir{0}; dir < 8; ++dir)
		{
			if (const uint64_t att{sq}; !(att & pattern[dir].border))
			{
				knight_table[i] |= shift(att, pattern[dir].shift);
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
