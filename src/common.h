#pragma once
#include <string>

#pragma warning(disable: 4244)

const std::string eng_name{ "Axon" };
const std::string version { "0.1" };
const std::string platform{ "x64" };

constexpr int max_depth { 64 };
constexpr int max_hash{ 1024 };
constexpr int max_period { 1024 };
constexpr int max_movegen { 256 };
constexpr int max_movetime{ 0x7fffffff };

constexpr uint8_t castleright[]{ 0x1, 0x4, 0x10, 0x40 };
const std::string startpos{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0" };

inline uint16_t to_sq1(const uint16_t move) { return move & 0x3fU; }
inline uint16_t to_sq2(const uint16_t move) { return (move & 0xfc0U) >> 6; }
inline uint8_t to_flag(const uint16_t move) { return static_cast<uint8_t>(move >> 12); }
inline uint64_t shift(const uint64_t b, const int shift) { return b << shift | b >> (64 - shift); }
inline uint16_t encode(const uint32_t from, const uint32_t to, const int flag) { return static_cast<uint16_t>(from | to << 6 | flag << 12); }

enum square
{
	h1, g1, f1, e1, d1, c1, b1, a1,
	h2, g2, f2, e2, d2, c2, b2, a2,
	h3, g3, f3, e3, d3, c3, b3, a3,
	h4, g4, f4, e4, d4, c4, b4, a4,
	h5, g5, f5, e5, d5, c5, b5, a5,
	h6, g6, f6, e6, d6, c6, b6, a6,
	h7, g7, f7, e7, d7, c7, b7, a7,
	h8, g8, f8, e8, d8, c8, b8, a8
};

enum color
{
	white,
	black,
	both
};

enum piece
{
	pawns = 0,
	rooks = 1,
	knights = 2,
	bishops = 3,
	queens = 4,
	kings = 5,
	no_piece = 6
};

enum pawn
{
	enpassant = 7,
};

enum castle
{
	white_kingside = 8,
	white_queenside = 9,
	black_kingside = 10,
	black_queenside = 11
};

enum promote
{
	promo_rook = 12,
	promo_knight = 13,
	promo_bishop = 14,
	promo_queen = 15
};

enum castlerights
{
	white_short,
	white_long,
	black_short,
	black_long
};

enum hashtype
{
	exact = 1,
	upper = 2,
	lower = 3
};

enum mvgen
{
	quiets,
	captures,
	all
};

enum scoretype
{
	draw_score = 0,
	max_score = 9900,
	mate_score = 10000,
	ndef = 11000
};

enum slide
{
	rook,
	bishop
};
enum stage
{
	mg,
	eg
};

enum state
{
	active,
	checkmate,
	stalemate,
	isdraw
};
