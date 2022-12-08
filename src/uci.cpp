#include <cassert>
#include <thread>
#include <sstream>
#include "engine.h"
#include "notation.h"
#include "board.h"
#include "uci.h"
#include "bench.h"

void uci::loop()
{
	std::string line, token;
	timemanager clock;
	std::thread searching;
	board pos{};
	pos.parse_fen(startpos);

	do
	{
		std::getline(std::cin, line);

		std::istringstream input(line);
		input >> token;

		if (line == "uci")
		{
			uci();
		}
		else if (line == "stop")
		{
			stop(searching);
		}
		else if (line == "isready")
		{
			isready();
		}
		else if (line == "ucinewgame")
		{
			stop(searching);
			engine::new_game(pos, clock);
		}
		else if (token == "setoption")
		{
			stop(searching);
			input >> token;
			std::string name;
			input >> name;
			if (name == "hash" || name == "Hash")
			{
				input >> token;
				if (input >> token)
					engine::new_hash_size(stoi(token));
			}
		}
		else if (token == "position")
		{
			stop(searching);
			input >> token;
			if (token == "startpos")
			{
				if (input.peek() == EOF)
				{
					engine::new_game(pos, clock);
					continue;
				}
			}
			else if (token == "fen")
			{
				std::string fen;
				while (input >> token && token != "moves")
					fen += token + " ";

				if (token != "moves")
				{
					engine::parse_fen(pos, clock, fen);
					continue;
				}
				if (input >> token && input.peek() == EOF)
				{
					engine::parse_fen(pos, clock, fen);
				}
			}

			while (input.peek() != EOF)
				input >> token;

			const char promo = token.back();
			if (promo == 'q' || promo == 'r' || promo == 'n' || promo == 'b')
				token.pop_back();

			assert(token.size() == 4);

			uint64_t from = notation::conv_to_bb(token.substr(0, 2));
			uint64_t to = notation::conv_to_bb(token.substr(2, 2));
			const uint8_t flag = notation::to_flag(promo, pos, from, to);

			engine::new_move(pos, from, to, flag);
		}
		else if (token == "perft")
		{
			stop(searching);
			if (input >> token)
			{
				const int depth = stoi(token);
				bench::root_perft(pos, depth);
			}
			else
				bench::root_perft(pos, 6);
		}
		else if (token == "go")
		{
			while (input >> token)
			{
				if (token == "infinite")
				{
					engine::depth = max_depth;
					clock.set_movetime(max_movetime);
				}
				else if (token == "depth")
				{
					input >> token;
					engine::depth = stoi(token);
					if (engine::depth > max_depth)
						engine::depth = max_depth;
				}
				else if (token == "movetime")
				{
					input >> token;
					clock.set_movetime(stoi(token));
				}
				else if (token == "wtime")
				{
					input >> token;
					clock.time[white] = stoi(token);
					clock.movetime_is_set = false;
				}
				else if (token == "btime")
				{
					input >> token;
					clock.time[black] = stoi(token);
					clock.movetime_is_set = false;
				}
				else if (token == "winc")
				{
					input >> token;
					clock.incr[white] = stoi(token);
					clock.movetime_is_set = false;
				}
				else if (token == "binc")
				{
					input >> token;
					clock.incr[black] = stoi(token);
					clock.movetime_is_set = false;
				}
				else if (token == "movestogo")
				{
					input >> token;
					clock.moves_to_go = stoi(token);
					clock.movetime_is_set = false;
				}
			}
			searching = std::thread{search, &pos, &clock};
		}
	}
	while (line != "quit");

	stop(searching);
}

void uci::isready()
{
	std::cout << "readyok" << std::endl;
}

void uci::search(board* pos, timemanager* chrono)
{
	const auto best_move = engine::alphabeta(*pos, *chrono);

	assert(best_move != 0);

	const uint64_t from = 1ULL << to_sq1(best_move);
	const uint64_t to = 1ULL << to_sq2(best_move);
	const uint8_t flag = to_flag(best_move);

	engine::new_move(*pos, from, to, flag);

	std::cout << "bestmove "
		<< notation::conv_to_str(from)
		<< notation::conv_to_str(to)
		<< notation::to_promotion(flag)
		<< std::endl;
}

void uci::stop(std::thread& searching)
{
	if (searching.joinable())
	{
		engine::stop = true;
		searching.join();
	}
}

void uci::uci()
{
	std::cout
		<< "id name" << " " << eng_name << " " << version << " " << platform << std::endl
		<< "id author Jasper Sinclair" << std::endl << std::endl
		<< "option name Hash type spin default " << engine::hash_size << " min 1 max " << 1024 << std::endl;
	std::cout
		<< "uciok" << std::endl;
}
