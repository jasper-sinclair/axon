#include <cassert>
#include <iostream>
#include "bench.h"
#include "movegen.h"
#include "timeman.h"

uint64_t total_nodes = 0;

uint64_t bench::perft(board& pos, const int depth)
{
	uint64_t nodes{0};

	if (depth == 0) return 1;

	const movegen gen(pos, all);
	const board save(pos);

	for (int i{0}; i < gen.move_cnt; ++i)
	{
		pos.new_move(gen.movelist[i]);
		nodes += perft(pos, depth - 1);
		pos = save;
	}

	return nodes;
}

void bench::root_perft(board& pos, const int depth)
{
	assert(depth >= 1);
	assert(depth >= 1);

	for (int d = 1; d <= depth; ++d)
	{
		const timer time;
		const uint64_t nodes{perft(pos, d)};
		total_nodes += nodes;
		std::cout << "depth " << d << " nodes " << nodes << " time " << time.elapsed()
			<< " nps " << nodes / (static_cast<uint64_t>(time.elapsed()) + 1) * 1000 << std::endl;
	}
}
