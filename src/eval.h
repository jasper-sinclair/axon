#pragma once
#include "board.h"

namespace eval
{
	void init_phases();
	int material(const board& pos);
	int piece_squares(const board& pos);
	int position(board& pos);
	void pieces(board& pos, int sum[][2]);

	constexpr int piece_value[2][6]
	{
		{90, 500, 320, 330, 980, 0},
		{100, 550, 320, 350, 1000, 0}
	};

	constexpr int tempo_bonus_white{ 10 };
	constexpr int bishop_pair[2] { 30, 60 };
}
