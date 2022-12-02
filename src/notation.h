#pragma once
#include "board.h"
#include "common.h"

namespace notation
{
	int conv_to_int(const std::string& sq);
	std::string conv_to_san(board& pos, const uint64_t& from_64, const uint64_t& to_64, uint8_t flag);
	std::string conv_to_str(const uint64_t& sq);
	std::string conv_to_str(int sq);
	std::string to_promotion(uint8_t flag);
	uint64_t conv_to_bb(const std::string& sq);
	uint8_t to_flag(char promo, const board& pos, const uint64_t& from_64, const uint64_t& to_64);
}

inline constexpr uint8_t piece_char[]{'R', 'N', 'B', 'Q', 'K'};
