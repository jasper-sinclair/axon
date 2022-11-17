#include <cassert>
#include "engine.h"
#include "bitops.h"
#include "eval.h"
#include "game.h"
#include "hash.h"
#include "magic.h"
#include "movegen.h"
#include "notation.h"
#include "random.h"
#include "search.h"

void engine::new_game(board& pos, timemanager& chrono)
{
	game::reset();
	pos.parse_fen(startpos);
	depth = max_depth;
	chrono.set_movetime(max_movetime);
	hash::tt_clear();
}

void engine::new_move(board& pos, const uint64_t& from_64, const uint64_t& to_64, const uint8_t flag)
{
	if (const int control{game::moves / 2 + 1}; game::moves != control)
		game::game_str += std::to_string(control) + ". ";

	game::game_str += notation::convert_to_san(pos, from_64, to_64, flag) + " ";

	const int from{lsb(from_64)};
	const int to{lsb(to_64)};

	const uint16_t move{encode(from, to, flag)};
	pos.new_move(move);

	assert(to_64 & pos.side[pos.turn ^ 1]);
	game::save_move(pos, move);
}

void engine::parse_fen(board& pos, timemanager& chrono, const std::string& fen)
{
	game::reset();
	depth = max_depth;
	chrono.set_movetime(max_movetime);
	hash::tt_clear();
	pos.parse_fen(fen);
}

void engine::init_movegen()
{
	movegen::init();
	magic::init_seeds();
	magic::init_magic();
	magic::init_ray(rook);
	magic::init_ray(bishop);
	magic::init_king();
	magic::init_knight();
}

void engine::init_eval() { eval::init_phases(); }
void engine::init_rand() { srand(static_cast<uint16_t>(random_u64_number())); }
void engine::init_hash(const int size) { hash::tt_create(size); }
uint16_t engine::alphabeta(board& pos, timemanager& chrono) { return search::idd(pos, chrono); }
