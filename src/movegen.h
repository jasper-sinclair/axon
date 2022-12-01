#pragma once
#include <vector>
#include "board.h"
#include "common.h"

class movegen
{
public:
	movegen() = default;

	movegen(const board& pos, const gentype type)
	{
		gen_moves(pos, type);
	}

	class legal
	{
	public:
		legal() = default;

		explicit legal(const board& pos)
		{
			init_legal(pos);
			pin(pos);
		}

		~legal()
		{
			unpin();
		}

		static void pin_down(const board& pos);
	};

	bool in_list(uint16_t move);
	int capt_cnt = 0;
	int move_cnt = 0;
	int promo_cnt = 0;
	static uint64_t king_table[64];
	static uint64_t knight_table[64];
	static uint64_t legal_sq;
	static uint64_t pinned[64];
	uint16_t movelist[max_movegen]{};
	uint16_t* find(uint16_t move);

	static void init();
	int gen_moves(const board& pos, gentype type);
	static uint64_t slider_attacks(int sl, int sq, uint64_t occ);
	static uint64_t check(const board& pos, int turn, uint64_t squares);

private:
	static void init_legal(const board& pos);
	static void pin(const board& pos);
	static void unpin();
	void king_moves(const board& pos, gentype type);
	void pawn_captures(const board& pos);
	void pawn_promo(const board& pos);
	void pawn_quiet(const board& pos);
	void piece_moves(const board& pos, gentype type);
};

inline uint64_t movegen::king_table[64]{0ULL};
inline uint64_t movegen::knight_table[64]{0ULL};
inline uint64_t movegen::legal_sq;
inline uint64_t movegen::pinned[64];
