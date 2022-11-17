#pragma once
#include "board.h"
#include "common.h"

class game
{
public:
	static int moves;
	static void reset();
	static uint16_t movelist[];
	static uint64_t hashlist[];
	static std::string game_str;
	static void save_move(const board& pos, uint16_t move);
};

namespace draw
{
	bool by_material(const board& pos);
	bool verify(const board& pos, uint64_t list[]);
	bool by_rep(const board& pos, uint64_t list[], int size);
}

inline int game::moves;
inline std::string game::game_str;
inline uint16_t game::movelist[max_period];
inline uint64_t game::hashlist[max_period];

inline bool lone_bishop(const board& pos) { return (pos.pieces[bishops] | pos.pieces[kings]) == pos.side[both]; }
inline bool lone_knight(const board& pos) { return (pos.pieces[knights] | pos.pieces[kings]) == pos.side[both]; }
constexpr uint64_t sqs[] { 0xaa55aa55aa55aa55, 0x55aa55aa55aa55aa };
