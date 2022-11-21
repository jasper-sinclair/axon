#pragma once
#include "board.h"

namespace bench
{
	uint64_t perft(board& pos, int depth);
	void root_perft(board& pos, int depth);
}

inline uint64_t total_nodes = 0;