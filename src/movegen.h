#pragma once
#include <vector>
#include "board.h"
#include "common.h"

class movegen
{
public:
	movegen() = default;
	movegen(const board& pos, const mvgen type) { gen_moves(pos, type); }
	int move_cnt{};
	static void init();
	int gen_moves(const board& pos, mvgen type);
	uint16_t movelist[max_movegen]{};

private:
	static void unpin();
	void pin(const board& pos) const;
	void pawn_promotions(const board& pos);
	void pawn_captures(const board& pos);
	void pawn_quiet_moves(const board& pos);
	static void legal(const board& pos);
	void piece_moves(const board& pos, mvgen type);
	void king_moves(const board& pos, mvgen type);

public:
	bool in_list(uint16_t move);
	uint16_t* find(uint16_t move);
	static uint64_t check(const board& pos, int turn, uint64_t squares);
	static uint64_t slider_attacks(uint8_t sl, int sq, uint64_t occ);
};

struct rays
{
	int shift;
	uint64_t border;
};

inline void shift_real(uint64_t& bb, const int shift)
{
	bb = bb << shift | bb >> (64 - shift);
}

inline uint64_t slide_ray[2][64];

inline uint64_t knight_table[64];
inline uint64_t king_table[64];

inline uint64_t pinned[64];
inline uint64_t legal_sq;

inline uint64_t enemies;
inline uint64_t friends;

inline uint64_t king_fr;
inline uint64_t pawn_rank;

inline uint64_t border_left;
inline uint64_t border_right;

inline uint64_t gentype[2];
inline uint32_t sq_king_fr;

inline uint8_t ahead;
inline uint8_t back;

inline uint32_t pin_idx[8];
inline int pin_cnt;

inline std::vector<uint64_t> attack_table;

constexpr int cap_left[] { 9, 55, -9 };
constexpr int cap_right[] { 7, 57, -7 };

constexpr int push[] { 8, 56, -8 };
constexpr int double_push[] { 16, 48, -16 };

constexpr int table_size { 107648 };

constexpr uint64_t bfile[] { 0x7f7f7f7f7f7f7f7f, 0xfefefefefefefefe };

constexpr uint64_t promo_rank { 0xff000000000000ff };
constexpr uint64_t sec_rank[] { 0xff0000, 0xff0000000000 };

constexpr rays ray[]
{
	{8, 0xff00000000000000}, {7, 0xff01010101010101},
	{63, 0x0101010101010101}, {55, 0x01010101010101ff},
	{56, 0x00000000000000ff}, {57, 0x80808080808080ff},
	{1, 0x8080808080808080}, {9, 0xff80808080808080}
};

constexpr uint64_t file[]
{
	0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
	0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
};

constexpr uint64_t rank[]
{
	0x00000000000000ff, 0x000000000000ff00, 0x0000000000ff0000, 0x00000000ff000000,
	0x000000ff00000000, 0x0000ff0000000000, 0x00ff000000000000, 0xff00000000000000
};

constexpr uint64_t border[]{ file[7], file[0] };

namespace attack
{
	uint64_t by_pawns(const board& pos, int col);
}