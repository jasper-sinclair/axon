#pragma once
#include "board.h"
#include "common.h"

namespace notation
{
	std::string conv_to_str(int sq);
	int convert_to_int(const std::string& sq);
	uint64_t conv_to_bb(const std::string& sq);
	std::string to_promotion(uint8_t flag);
	std::string conv_to_str(const uint64_t& sq);
	std::string convert_to_san(board& pos, const uint64_t& from_64, const uint64_t& to_64, uint8_t flag);
	uint8_t to_flag(char promo, const board& pos, const uint64_t& from_64, const uint64_t& to_64);
}

constexpr uint8_t piece_char[] {'R', 'N', 'B', 'Q', 'K'};
constexpr uint64_t file_h { 0x101010101010101 };
constexpr uint64_t rank_1 { 0xff };
