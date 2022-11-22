#include <cassert>
#include "bitops.h"
#include "engine.h"
#include "eval.h"
#include "game.h"
#include "hash.h"
#include "magic.h"
#include "movegen.h"
#include "search.h"

namespace
{
	hash table(engine::hash_size);
}

uint16_t engine::alphabeta(board& pos, timemanager& chrono)
{
	stop = false;
	analysis::reset();
	return search::id_frame(pos, chrono);
}

void engine::init_eval()
{
	eval::init();
}

void engine::init_movegen()
{
	movegen::init();
	magic::init_seeds();
	magic::init_magic();
	magic::init_ray(rook_slider);
	magic::init_ray(bishop_slider);
	magic::init_king();
	magic::init_knight();
}

void engine::init_rand()
{
	srand(static_cast<uint16_t>(random_u64_number()));
}

void engine::new_game(board& pos, timemanager& chrono)
{
	parse_fen(pos, chrono, startpos);
}

void engine::new_hash_size(const int size)
{
	hash_size = hash::create(size);
}

void engine::new_move(board& pos, const uint64_t& from_64, const uint64_t& to_64, const uint8_t flag)
{
	const int from = lsb(from_64);
	const int to = lsb(to_64);

	const uint16_t move = encode(from, to, flag);
	pos.new_move(move);

	assert(to_64 & pos.side[pos.turn ^ 1]);

	game::save_move(pos, move);
}

void engine::parse_fen(board& pos, timemanager& chrono, const std::string& fen)
{
	game::reset();
	depth = max_depth;
	chrono.set_movetime(max_movetime);
	hash::clear();
	pos.parse_fen(fen);
}
