#pragma once
#include <ctime>
#include "common.h"

class rand_xor
{
	uint64_t seed_;
	uint64_t rand64();

public:
	explicit rand_xor(const uint64_t new_seed) : seed_(new_seed)
	{
	}
	uint64_t sparse64();
};

static unsigned int random_state = static_cast<unsigned int>(time(nullptr));
unsigned int get_random_u32_number();
uint64_t random_u64_number();
