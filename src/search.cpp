#include <cassert>
#include "sort.h"
#include "eval.h"
#include "hash.h"
#include "notation.h"
#include "engine.h"
#include "game.h"
#include "search.h"

int search::alphabeta(board& pos, const int ply, const int depth, int beta, int alpha)
{
	assert(beta > alpha);

	hashlist[pos.moves - 1] = pos.key;
	if (draw::verify(pos, hashlist, depth))
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
		if (beta <= alpha)
			return alpha;
	}

	struct hashtable
	{
		int score = ndef;
		uint16_t move = 0;
		uint8_t flag = 0;
	} t;

	if (hash::probe(pos, t.move, t.score, ply, depth, t.flag))
	{
		assert(t.score != scoretype::ndef);
		assert(t.flag != 0);

		if (t.score >= beta || t.score <= alpha)
		{
			if (t.flag == upper && t.score >= beta) return t.score;
			if (t.flag == lower && t.score <= alpha) return t.score;
			if (t.flag == exact) return t.score;
		}
	}
	if (t.flag != exact || t.score <= alpha || t.score >= beta)
		t.move = 0;

	const bool skip_pruning = pv_evol[last][depth] || no_pruning[depth] || is_check(pos);
	int score = ndef;

	if (ply <= 3 && !skip_pruning)
		score = eval::position(pos);

	if (ply <= 3
		&& abs(beta) < max_score
		&& !skip_pruning
		&& score - ply * 50 >= beta)
	{
		assert(score != scoretype::ndef);
		return score;
	}

	if (ply <= 3
		&& !skip_pruning
		&& score + ply * 50 + 100 <= alpha)
	{
		const auto r_alpha = alpha - ply * 50 - 100;

		if (const auto new_s = qsearch(pos, r_alpha, r_alpha + 1); new_s <= r_alpha)
			return new_s;
	}

	if (ply >= 3
		&& !skip_pruning
		&& beta != draw_score
		&& !pos.lone_king()
		&& eval::position(pos) >= beta)
	{
		constexpr int R = 3;
		uint64_t ep_copy = 0;

		pos.null_move(ep_copy);

		no_pruning[depth + 1] = true;
		score = -alphabeta(pos, ply - R, depth + 1, 1 - beta, -beta);
		no_pruning[depth + 1] = false;

		if (engine::stop) return 0;

		pos.undo_null_move(ep_copy);

		if (score >= beta)
		{
			hash::store(pos, 0, score, ply, depth, upper);
			return score;
		}
	}

	movegen gen(pos, all);
	const uint16_t* best_move = nullptr;

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
		if (const uint16_t* pos_list = gen.find(pv_evol[last][depth]); pos_list != gen.movelist + gen.move_cnt)
			best_move = pos_list;

		pv_evol[last][depth] = 0;
	}

	else if (t.move != 0)
	{
		if (const uint16_t* pos_list = gen.find(t.move); pos_list != gen.movelist + gen.move_cnt)
			best_move = pos_list;
	}

	sort list(pos, gen, best_move, history, killer, depth);
	const board save(pos);
	const auto old_alpha = alpha;

	for (auto move = list.next(gen); move; move = list.next(gen))
	{
		pos.new_move(move);

		int ext = 1;
		if (is_check(pos) && ply <= 4)
			ext = 0;

		score = -alphabeta(pos, ply - ext, depth + 1, -alpha, -beta);

		if (engine::stop) return 0;
		pos = save;

		if (score >= beta)
		{
			hash::store(pos, 0, score, ply, depth, upper);
			update_heuristics(pos, move, ply, depth);
			return score;
		}
		if (score > alpha)
		{
			alpha = score;
			hash::store(pos, move, score, ply, depth, exact);

			pv_evol[depth - 1][0] = move;
			for (int i = 0; pv_evol[depth][i] != 0; ++i)
			{
				pv_evol[depth - 1][i + 1] = pv_evol[depth][i];
			}
		}
	}

	if (alpha == old_alpha)
		hash::store(pos, 0, alpha, ply, depth, lower);

	return alpha;
}

uint16_t search::id_frame(board& pos, timemanager& chrono)
{
	assert(engine::depth >= 1);
	assert(engine::depth <= max_depth);

	spare_time.manage.start();
	spare_time.max = chrono.get_movetime(pos.turn);
	assert(spare_time.max > 0);

	uint16_t pv[max_depth]{0};
	uint16_t best_move = 0;

	for (int i = abs(game::moves - 2); i <= game::moves; ++i)
		hashlist[i] = game::hashlist[i];

	for (auto& i : pv_evol) for (auto& p : i) p = 0;
	for (auto& i : killer) for (auto& k : i) k = 0;
	for (auto& i : history) for (auto& j : i) for (auto& h : j) h = 1;

	root.gen_moves(pos, all);
	if (root.move_cnt == 1)
		return root.movelist[0];

	for (int ply = 1; ply <= engine::depth; ++ply)
	{
		timer mean_time;
		board::nodes = 0;
		int score = 0;

		root_alphabeta(pos, pv, score, ply);
		const auto interim = mean_time.elapsed();

		if (pv[0])
		{
			best_move = pv[0];

			ebf.all_nodes += board::nodes;
			if (ebf.prev_nodes != 0.0)
			{
				ebf.factor += static_cast<double>(board::nodes) / ebf.prev_nodes;
				ebf.count += 1;
			}
			ebf.prev_nodes = static_cast<double>(board::nodes);

			assert(pos.nodes != 0);
			const auto nps = static_cast<int>(board::nodes * 1000 / (interim + 1));

			auto seldepth = ply;
			for (; seldepth < max_depth; ++seldepth)
			{
				if (pv[seldepth] == 0)
					break;
			}
			auto real_depth = ply;
			if (engine::stop)
				real_depth = ply - 1;

			for (int i = mate_score - abs(score); i < seldepth; pv[i++] = 0);

			if (score == draw_score)
			{
				const board saved(pos);
				for (int i = 0; pv[i] != 0; ++i)
				{
					if (movegen gen(pos, all); !gen.in_list(pv[i]))
					{
						for (auto j = i; pv[j] != 0; pv[j++] = 0);
						break;
					}
					pos.new_move(pv[i]);
				}
				pos = saved;
			}

			std::string score_str = "cp " + std::to_string(score);
			if (abs(score) >= max_score)
				score_str = "mate " + std::to_string((mate_score - abs(score) + 1) / 2);

			std::cout << "info"
				<< " depth " << real_depth
				<< " seldepth " << seldepth
				<< " score " << score_str
				<< " nodes " << board::nodes
				<< " nps " << nps
				<< " time " << interim
				<< " pv ";

			for (int i = 0; i < ply && pv[i] != 0; ++i)
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

	auto score = eval::position(pos);

	if (!gen.move_cnt) return score;
	if (score >= beta) return score;
	if (score > alpha) alpha = score;

	sort list(pos, gen);
	const board save(pos);

	for (auto move = list.next(gen); move; move = list.next(gen))
	{
		if (!pos.lone_king()
			&& !is_promo(move)
			&& score + eval::value[mg][pos.piece_sq[to_sq2(move)]] + 100 < alpha)
			continue;

		pos.new_move(move);

		score = -qsearch(pos, -beta, -alpha);
		pos = save;

		if (score >= beta) return beta;
		if (score > alpha) alpha = score;
	}
	return alpha;
}

void search::root_alphabeta(board& pos, uint16_t pv[], int& best_score, const int ply)
{
	const uint16_t* best_move = nullptr;

	if (pv[0])
	{
		assert(root.in_list(pv[0]));

		best_move = root.find(pv[0]);

		for (int i = 0; pv[i] != 0; ++i)
			pv_evol[last][i] = pv[i];
		pv[0] = 0;
	}

	sort list(pos, root, best_move, history);
	const board saved(pos);

	constexpr int beta = mate_score;
	int alpha = -beta;

	for (auto move = list.next(root); move && !engine::stop; move = list.next(root))
	{
		assert(beta > alpha);
		assert(root.in_list(move));

		pos.new_move(move);

		const int score = -alphabeta(pos, ply - 1, 1, -alpha, -beta);
		pos = saved;

		if (score > alpha && !engine::stop)
		{
			alpha = score;
			pv[0] = move;
			for (int i = 0; pv_evol[0][i] != 0; ++i)
			{
				pv[i + 1] = pv_evol[0][i];
			}

			hash::store(pos, pv[0], score, ply, 0, exact);
		}
	}

	best_score = alpha;
}

void analysis::reset()
{
	ebf.all_nodes = 0;
	ebf.prev_nodes = 0;
	ebf.factor = 0;
	ebf.count = 0;
}
