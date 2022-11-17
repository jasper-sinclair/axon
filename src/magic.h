#pragma once
#include <cstdint>
#include <vector>

namespace magic
{
	struct magics
	{
		size_t offset;
		uint64_t mask;
		uint64_t magic;
		int shift;
	};

	inline uint64_t seeds[16];
	inline magics slider[2][64];
	void init_seeds();
	void init_magic();
	void init_king();
	void init_knight();
	void init_ray(uint8_t sl);
	void init_mask(uint8_t sl);
	void init_blocker(uint8_t sl, std::vector<uint64_t>& blocker);
	void init_move(uint8_t sl, const std::vector<uint64_t>& blocker, std::vector<uint64_t>& attack_temp);
	void init_magic(uint8_t sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp);
	void init_connect(uint8_t sl, const std::vector<uint64_t>& blocker, const std::vector<uint64_t>& attack_temp);
}

