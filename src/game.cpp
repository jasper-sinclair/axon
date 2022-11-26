#include "bitops.h"
#include "game.h"

bool draw::by_material(const board& pos)
{
	if (lone_bishops(pos) && (!(sqs[white] & pos.pieces[bishop]) || !(sqs[black] & pos.pieces[bishop])))
		return true;

	if (lone_knights(pos) && popcnt(pos.pieces[knight]) == 1)
		return true;

	return false;
}

bool draw::by_rep(const board& pos, uint64_t list[], const int depth)
{
	const int size = game::moves + depth - 1;
	for (int i = 4; i <= pos.half_moves && i <= size; i += 2)
	{
		if (list[size - i] == list[size])
			return true;
	}

	return false;
}

void game::reset()
{
	moves = 0;
	for (auto& m : movelist) m = 0;
	for (auto& h : hashlist) h = 0;
}

void game::save_move(const board& pos, const uint16_t move)
{
	movelist[moves] = move;
	hashlist[moves] = pos.key;
	moves += 1;
}

bool draw::verify(const board& pos, uint64_t list[], const int depth)
{
	if (pos.half_moves >= 4 && by_rep(pos, list, depth))
		return true;
	if (by_material(pos))
		return true;
	if (pos.half_moves == 100)
		return true;
	return false;
}
