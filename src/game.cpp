#include "bitops.h"
#include "movegen.h"
#include "game.h"

bool draw::by_material(const board& pos)
{
	if (lone_bishop(pos) && (!(sqs[white] & pos.pieces[bishops]) || !(sqs[black] & pos.pieces[bishops])))
		return true;
	if (lone_knight(pos) && popcnt(pos.pieces[knights]) == 1)
		return true;
	return false;
}

bool draw::by_rep(const board& pos, uint64_t list[], int size)
{
	size -= 1;
	for (int i{4}; i <= pos.half_moves && i <= size; i += 2)
	{
		if (list[size - i] == list[size])
			return true;
	}
	return false;
}

bool draw::verify(const board& pos, uint64_t list[])
{
	if (pos.half_moves >= 4 && by_rep(pos, list, pos.moves))
		return true;
	if (by_material(pos))
		return true;
	if (pos.half_moves >= 50)
		return true;
	return false;
}

void game::reset()
{
	moves = 0;
	game_str.clear();
	for (auto& m : movelist) m = 0;
	for (auto& h : hashlist) h = 0;
}

void game::save_move(const board& pos, const uint16_t move)
{
	movelist[moves] = move;
	hashlist[moves] = pos.key;
	moves += 1;
}