#pragma once
#include "board.h"

namespace eval
{
	int position(board& pos);
	void init();
	void pawns(const board& pos, int sum[][2]);
	void pieces(board& pos, int sum[][2]);

	constexpr int value[2][6]
	{
		{90, 500, 320, 330, 970, 0},
		{100, 550, 320, 350, 1000, 0}
	};

	constexpr int tempo_bonus[]{20, 0};
	constexpr int bishop_pair[2]{30, 60};
	constexpr int knights_connected[2]{10, 10};
	constexpr int major_behind_pp[2]{10, 20};
	constexpr int rook_open_file{8};

	constexpr int king_safety_w[8]
	{
		0, 0, 50, 75, 88, 94, 97, 99
	};
	constexpr int king_threat[5]
	{
		0, 40, 20, 20, 80
	};

	static int passed_pawn[2][64]
	{
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			150, 130, 120, 110, 110, 120, 130, 150,
			104, 96, 88, 80, 80, 88, 96, 104,
			70, 65, 60, 55, 55, 60, 65, 70,
			50, 45, 40, 35, 35, 40, 45, 50,
			28, 25, 23, 20, 20, 23, 25, 28,
			15, 13, 12, 10, 10, 12, 13, 15,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	};

	constexpr int bishop_mob[2][14]
	{
		{-15, -10, 0, 10, 18, 25, 30, 34, 37, 39, 40, 41, 42, 43},
		{-15, -10, 0, 10, 18, 25, 30, 34, 37, 39, 40, 41, 42, 43}
	};
	constexpr int rook_mob[2][15]
	{
		{-8, -5, -3, 0, 3, 5, 8, 10, 12, 13, 14, 15, 16, 17, 18},
		{-18, -10, -4, 2, 8, 14, 20, 26, 32, 37, 40, 42, 43, 44, 45}
	};
	constexpr int queen_mob[2][28]
	{
		{
			-4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 6, 7, 7,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
		},
		{
			-10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 13, 14,
			14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14
		}
	};
	constexpr int knight_mob[2][9]
	{
		{-15, -10, -5, 0, 5, 8, 10, 12, 13},
		{-15, -10, -5, 0, 5, 8, 10, 12, 13}
	};
}

inline uint64_t front_span[2][64]{};
constexpr int negate[]{ 1, -1 };
constexpr int phase_max = 24;
inline int phase_weight[phase_max + 1]{};
