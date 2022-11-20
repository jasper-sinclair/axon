# axon
a minimalist UCI chess engine in std::C++17

- alpha-beta search
- basic move ordering
- transposition table
- magic bitboard move generation
- incorporating core engine components only
- basic eval: material & piece square tables
- current size 72 KB
- optimized & stable
- accurate perft
- portable
- uci options: Hash type spin default 128 min 1 max 1024
- command-line options: perft <depth>
- playing strength ~1900 elo
