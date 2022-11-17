#include <cassert>
#include <iostream>
#include "eval.h"
#include "hash.h"
#include "notation.h"
#include "movegen.h"
#include "engine.h"
#include "game.h"
#include "search.h"

int search::alphabeta(board& pos, const int ply, const int depth, int beta, int alpha)
{
	assert(beta > alpha);

	hashlist[pos.moves - 1] = pos.key;
	if (draw::verify(pos, hashlist))
		return draw_score;

	if (ply == 0 || depth >= max_depth)
		return qsearch(pos, alpha, beta);

	if (ply > 1 && time_is_up())
	{
		engine::stop = true;
		return 0;
	}

	if (mate_score - depth < beta)
	{
		beta = mate_score - depth;
		if (mate_score - depth <= alpha)
			return alpha;
	}

	const bool skip_pruning{ is_check(pos) || pv_evol[last][depth] || no_pruning[depth] };
	int score{ ndef };

	uint16_t tt_move{ 0 };
	uint8_t tt_flag{ 0 };
	if (hash::tt_probe(ply, pos, score, tt_move, tt_flag))
	{
		assert(score != scoretype::ndef);
		assert(tt_move != 0);
		assert(tt_flag != 0);

		if (score > max_score) score -= depth;
		if (score < -max_score) score += depth;

		if (score >= beta || score <= alpha)
		{
			if (tt_flag == upper && score >= beta) return score;
			if (tt_flag == lower && score <= alpha) return score;
			if (tt_flag == exact) return score;
		}
	}
	if (tt_flag != exact || score <= alpha || score >= beta)
		tt_move = 0;

	if (!skip_pruning)
	{
		if (ply <= 3)
			score = eval::position(pos);

		if (ply <= 3
			&& abs(beta) < max_score
			&& score - ply * 50 >= beta)
		{
			assert(score != scoretype::ndef);
			return score;
		}

		if (ply <= 3
			&& score + ply * 50 + 100 <= alpha)
		{
			const int r_alpha{ alpha - ply * 50 - 100 };

			if (const int new_s{ qsearch(pos, r_alpha, r_alpha + 1) }; new_s <= r_alpha)
				return new_s;
		}

		if (ply >= 2
			&& !pos.lone_king()
			&& (score != ndef && score >= beta || score == ndef && eval::position(pos) >= beta))
		{
			const int R { ply > 6 ? 3 : 2 };
			uint64_t ep_copy{ 0 };

			pos.null_move(ep_copy);

			no_pruning[depth + 1] = true;
			score = -alphabeta(pos, ply - R, depth + 1, 1 - beta, -beta);
			no_pruning[depth + 1] = false;

			if (engine::stop) return 0;

			pos.undo_null_move(ep_copy);

			if (score >= beta) return score;
		}
	}

	movegen gen(pos, all);

	if (!gen.move_cnt)
	{
		if (is_check(pos))
			alpha = depth - mate_score;
		else
			alpha = draw_score;

		return alpha;
	}

	if (pv_evol[last][depth])
	{
		if (uint16_t* pos_list{ gen.find(pv_evol[last][depth]) }; pos_list != gen.movelist + gen.move_cnt)
			std::swap(gen.movelist[0], *pos_list);
		else
			for (int i{ depth + 1 }; pv_evol[last][i] != 0; pv_evol[last][i++] = 0);

		pv_evol[last][depth] = 0;
	}

	else if (tt_move != 0)
	{
		if (uint16_t* pos_list{ gen.find(tt_move) }; pos_list != gen.movelist + gen.move_cnt)
			std::swap(gen.movelist[0], *pos_list);
	}

	const board save(pos);
	for (int nr{ 0 }; nr < gen.move_cnt; ++nr)
	{
		assert(gen.movelist[nr] != 0);

		pos.new_move(gen.movelist[nr]);

		int ext{ 1 };
		if (is_check(pos) && ply <= 4)
			ext = 0;

		score = -alphabeta(pos, ply - ext, depth + 1, -alpha, -beta);

		if (engine::stop) return 0;
		pos = save;

		if (score >= beta)
		{
			hash::tt_save(pos, gen.movelist[nr], score, ply, upper);
			return score;
		}
		if (score > alpha)
		{
			alpha = score;
			hash::tt_save(pos, gen.movelist[nr], score, ply, exact);

			pv_evol[depth - 1][0] = gen.movelist[nr];
			for (int i{ 0 }; pv_evol[depth][i] != 0; ++i)
			{
				pv_evol[depth - 1][i + 1] = pv_evol[depth][i];
			}
		}
		else
		{
			hash::tt_save(pos, gen.movelist[nr], score, ply, lower);
		}
	}
	return alpha;
}

void search::root_alphabeta(board& pos, uint16_t pv[], int& best_score, const int ply)
{
	if (pv[0])
	{
		assert(root.in_list(pv[0]));

		std::swap(root.movelist[0], root.movelist[1]);
		std::swap(root.movelist[0], *root.find(pv[0]));

		for (int i{ 0 }; pv[i] != 0; ++i)
		{
			pv_evol[last][i] = pv[i];
		}
	}

	const board saved(pos);
	int alpha{ -mate_score};
	constexpr int beta{mate_score};

	for (int nr{ 0 }; nr < root.move_cnt && !engine::stop; ++nr)
	{
		assert(root.movelist[nr] != 0);
		assert(beta > alpha);

		pos.new_move(root.movelist[nr]);

		const int score = -alphabeta(pos, ply - 1, 1, -alpha, -beta);
		pos = saved;

		if (score > alpha && !engine::stop)
		{
			alpha = score;
			pv[0] = root.movelist[nr];
			for (int i{ 0 }; pv_evol[0][i] != 0; ++i)
			{
				pv[i + 1] = pv_evol[0][i];
			}

			hash::tt_save(pos, pv[0], score, ply, lower);
		}
	}

	if (alpha == -mate_score)
	{
		pv[0] = 0;
		return;
	}

	best_score = alpha;
	hash::tt_save(pos, pv[0], best_score, ply, exact);
}

uint16_t search::idd(board& pos, timemanager& chrono)
{
	assert(engine::depth >= 1);
	assert(engine::depth <= engine::depth);

	total_time.start();
	max_time = chrono.get_movetime(pos.turn);

	uint16_t pv[max_depth]{ 0 };
	uint16_t best_move{ 0 };
	std::string score_str;

	for (int i{ abs(game::moves - 2) }; i <= game::moves; ++i)
		hashlist[i] = game::hashlist[i];

	for (auto& i : pv_evol)
	{
		for (auto& p : i) p = 0;
	}

	root.gen_moves(pos, all);
	if (root.move_cnt == 1)
	{
		return root.movelist[0];
	}

	for (int ply{ 1 }; ply <= engine::depth; ++ply)
	{
		timer mean_time;
		board::nodes = 0;
		int score{ 0 };

		root_alphabeta(pos, pv, score, ply);
		const auto interim{ mean_time.elapsed() };

		if (pv[0])
		{
			best_move = pv[0];
			assert(pos.nodes != 0);
			const auto nps{ static_cast<int>(board::nodes * 1000 / (interim + 1)) };
			std::string nodes_str{ std::to_string(board::nodes) };

			auto seldepth{ ply };
			for (; seldepth < max_depth; ++seldepth)
			{
				if (pv[seldepth] == 0)
					break;
			}
			std::string ply_str = std::to_string(ply);
			if (engine::stop) ply_str = std::to_string(ply - 1);

			for (int i{ mate_score - abs(score) }; i < seldepth; pv[i++] = 0);

			if (score == draw_score)
			{
				const board saved(pos);
				for (int i{ 0 }; pv[i] != 0; ++i)
				{
					if (movegen gen(pos, all); !gen.in_list(pv[i]))
					{
						for (auto j{ i }; pv[j] != 0; pv[j++] = 0);
						break;
					}
					pos.new_move(pv[i]);
				}
				pos = saved;
			}

			if (abs(score) >= max_score)
				score_str = "mate " + std::to_string((mate_score - abs(score) + 1) / 2);
			else
				score_str = "cp " + std::to_string(score);

			std::cout << "info"
				<< " depth " << ply_str
				<< " seldepth " << std::to_string(seldepth)
				<< " score " << score_str
				<< " nodes " << nodes_str
				<< " nps " << nps
				<< " time " << interim
				<< " pv ";

			for (int i{ 0 }; i < ply && pv[i] != 0; ++i)
			{
				std::cout
					<< notation::conv_to_str(to_sq1(pv[i]))
					<< notation::conv_to_str(to_sq2(pv[i]))
					<< notation::to_promotion(to_flag(pv[i]));
			}
			std::cout << std::endl;

			if (engine::stop || time_is_up() || score > max_score)
				break;

			if (ply >= 5 && pv[ply - 5] == 0)
				break;
		}
		else break;
	}
	return best_move;
}

int search::qsearch(board& pos, int alpha, const int beta)
{
	if (draw::by_material(pos))
		return draw_score;

	const movegen gen(pos, captures);

	int score{ eval::position(pos) };
	if (!gen.move_cnt) return score;

	if (score >= beta) return score;
	if (score > alpha) alpha = score;

	const board save(pos);
	for (int nr{ 0 }; nr < gen.move_cnt; ++nr)
	{
		if (const int pce{ pos.piece_sq[to_sq2(gen.movelist[nr])] }; !pos.lone_king()
			&& !is_promo(gen.movelist[nr])
			&& score + eval::piece_value[mg][pce] + 100 < alpha)
			continue;

		pos.new_move(gen.movelist[nr]);

		score = -qsearch(pos, -beta, -alpha);
		pos = save;

		if (score >= beta) return beta;
		if (score > alpha) alpha = score;
	}
	return alpha;
}