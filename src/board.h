#pragma once
#include "common.h"

class board
{
public:
	int half_moves;
	int king_sq[2];
	int moves;
	int turn;
	static uint64_t nodes;
	uint64_t ep_square;
	uint64_t key;
	uint64_t pieces[6];
	uint64_t side[3];
	uint8_t castle_rights;
	uint8_t phase;
	uint8_t piece_sq[64];

	[[nodiscard]] bool lone_king() const;
	void clear();
	void new_move(uint16_t move);
	void null_move(uint64_t& ep_copy);
	void parse_fen(const std::string& fen);
	void rook_moved(const uint64_t& sq64, uint16_t sq);
	void undo_null_move(const uint64_t& ep_copy);
};

constexpr char p_char[][6]
{
	{'P', 'R', 'N', 'B', 'Q', 'K'},
	{'p', 'r', 'n', 'b', 'q', 'k'}
};

constexpr uint8_t castle_r[]{0xfa, 0xaf};
constexpr int ppush[]{8, 56};
constexpr int phase_value[]{0, 1, 1, 2, 4, 0, 0};

inline int mirror(const uint16_t sq)
{
	return (sq & 56) - (sq & 7) + 7;
}
