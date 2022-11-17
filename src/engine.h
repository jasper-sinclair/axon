#pragma once
#include "board.h"
#include "timeman.h"

class engine
{
public:
	static bool stop;
	static int depth;
	static int hash_size;
	static void init_eval();
	static void init_rand();
	static void init_movegen();
	static void init_hash(int size);
	static void new_game(board& pos, timemanager& chrono);
	static uint16_t alphabeta(board& pos, timemanager& chrono);
	static void parse_fen(board& pos, timemanager& chrono, const std::string& fen);
	static void new_move(board& pos, const uint64_t& from_64, const uint64_t& to_64, uint8_t flag);
};

inline bool engine::stop;
inline int engine::depth;
inline int engine::hash_size{ 128 };
