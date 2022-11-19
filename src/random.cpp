#include "random.h"

unsigned int get_random_u32_number()
{
	unsigned int number = random_state;

	number ^= number << 13;
	number ^= number >> 17;
	number ^= number << 5;

	random_state = number;
	return number;
}

uint64_t random_u64_number()
{
	const uint64_t n1 = static_cast<uint64_t>(get_random_u32_number()) & 0xFFFF;
	const uint64_t n2 = static_cast<uint64_t>(get_random_u32_number()) & 0xFFFF;
	const uint64_t n3 = static_cast<uint64_t>(get_random_u32_number()) & 0xFFFF;
	const uint64_t n4 = static_cast<uint64_t>(get_random_u32_number()) & 0xFFFF;
	return n1 | n2 << 16 | n3 << 32 | n4 << 48;
}

uint64_t rand_xor::rand64()
{
	seed_ ^= seed_ >> 12;
	seed_ ^= seed_ << 25;
	seed_ ^= seed_ >> 27;
	return seed_ * 0x2545f4914f6cdd1dULL;
}

uint64_t rand_xor::sparse64()
{
	return rand64() & rand64() & rand64();
}
