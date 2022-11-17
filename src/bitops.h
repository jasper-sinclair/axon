#pragma once
#include "common.h"

constexpr uint64_t k1 = 0x5555555555555555;
constexpr uint64_t k2 = 0x3333333333333333;
constexpr uint64_t k4 = 0x0f0f0f0f0f0f0f0f;
constexpr uint64_t kf = 0x0101010101010101;

inline int popcnt(uint64_t b)
{
	b = b - (b >> 1 & k1);
	b = (b & k2) + (b >> 2 & k2);
	b = b + (b >> 4) & k4;
	b = b * kf >> 56;
	return static_cast<int>(b);
}

inline int debruijn[64] = {
	0, 47, 1, 56, 48, 27, 2, 60,
	57, 49, 41, 37, 28, 16, 3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11, 4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30, 9, 24,
	13, 18, 8, 12, 7, 6, 5, 63
};

inline int lsb(const uint64_t b)
{
	return debruijn[ 0x03f79d71b4cb0a89 * (b ^ b - 1) >> 58 ];
}
