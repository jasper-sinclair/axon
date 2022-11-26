#pragma once
#include "board.h"
#include "common.h"

class game
{
public:
	static void save_move(const board& pos, uint16_t move);
	static void reset();
	static uint64_t hashlist[];
	static uint16_t movelist[];
	static int moves;
};

namespace draw
{
	bool by_material(const board& pos);
	bool by_rep(const board& pos, uint64_t list[], int depth);
	bool verify(const board& pos, uint64_t list[], int depth);
}

inline int game::moves;
inline uint16_t game::movelist[max_period];
inline uint64_t game::hashlist[max_period];

inline bool lone_bishops(const board& pos)
{
	return (pos.pieces[bishop] | pos.pieces[king]) == pos.side[both];
}

inline bool lone_knights(const board& pos)
{
	return (pos.pieces[knight] | pos.pieces[king]) == pos.side[both];
}

constexpr uint64_t sqs[]{0xaa55aa55aa55aa55, 0x55aa55aa55aa55aa};
