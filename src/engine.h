#pragma once
#include "board.h"
#include "timeman.h"

class engine
{
public:
	static int depth;
	static int hash_size;
	static bool stop;

	static uint16_t alphabeta(board& pos, timemanager& chrono);
	static void init_eval();
	static void init_movegen();
	static void init_rand();
	static void new_game(board& pos, timemanager& chrono);
	static void new_hash_size(int size);
	static void new_move(board& pos, const uint64_t& from_64, const uint64_t& to_64, uint8_t flag);
	static void parse_fen(board& pos, timemanager& chrono, const std::string& fen);
};

inline int engine::hash_size = 128;
inline int engine::depth;
inline bool engine::stop;