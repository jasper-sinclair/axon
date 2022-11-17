#pragma once

#include "common.h"

class board
{
public:
	int turn;
	int moves;
	int half_moves;

	uint64_t key;
	uint8_t phase;
	uint64_t ep_square;
	uint8_t castle_rights;

	uint64_t side[3];
	int piece_sq[64];
	uint64_t pieces[6];

	static uint64_t nodes;

	void clear();
	void new_move(uint16_t move);
	void parse_fen(const std::string& fen);
	void rook_moved(const uint64_t& sq);
	void null_move(uint64_t& ep_copy);
	void undo_null_move(const uint64_t& ep_copy);

	[[nodiscard]] bool lone_king() const;
};

constexpr char p_char[][6]
{
	{'P', 'R', 'N', 'B', 'Q', 'K'},
	{'p', 'r', 'n', 'b', 'q', 'k'}
};

inline uint64_t board::nodes;
constexpr uint8_t castler[] { 0xfa, 0xaf };
constexpr int pawnpush[] { 8, 56 };
constexpr int phase_value[] { 0, 1, 1, 2, 4, 0, 0 };
inline int mirror(const uint16_t sq) { return (sq & 56) - (sq & 7) + 7; }
