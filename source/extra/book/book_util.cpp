#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <inttypes.h>

#include "../../shogi.h"
#include "book.h"
#include "book_util.h"
#include "../../position.h"
#include "../../misc.h"
#include "../../search.h"
#include "../../thread.h"
#include "../../learn/multi_think.h"
#include "../../tt.h"

void is_ready();

#if defined(EVAL_LEARN) && (defined(YANEURAOU_2017_EARLY_ENGINE) || defined(YANEURAOU_2017_GOKU_ENGINE))

namespace BookUtil
{
	// 局面を与えて、その局面で思考させるために、やねうら王2017Early以降が必要。
	struct MultiThinkBook: public MultiThink
	{
		MultiThinkBook(int search_depth_, deqBook & book_)
			: search_depth(search_depth_), book(book_), appended(false) {}

		virtual void thread_worker(size_t thread_id);

		// 定跡を作るために思考する局面
		std::vector<std::string> sfens;

		// 定跡を作るときの通常探索の探索深さ
		int search_depth;

		// メモリ上の定跡ファイル(ここに追加していく)
		deqBook & book;

		// 前回から新たな指し手が追加されたかどうかのフラグ。
		bool appended;

		// 進捗表示の出力形式。
		int progress_type;

	};

	//  thread_id    = 0..Threads.size()-1
	//  search_depth = 通常探索の探索深さ
	void MultiThinkBook::thread_worker(size_t thread_id)
	{
		// g_loop_maxになるまで繰り返し
		u64 id;
		if (progress_type != 0)
			std::cout << ("info string progress " + std::to_string(get_loop_count()) + " / " + std::to_string(get_loop_max()) + "\n") << std::flush;
		while ((id = get_next_loop_count()) != UINT64_MAX)
		{
			auto sfen = sfens[id];

			auto & pos = Threads[thread_id]->rootPos;
			pos.set(sfen);
			auto th = Threads[thread_id];
			pos.set_this_thread(th);

			if (pos.is_mated())
				continue;

			// depth手読みの評価値とPV(最善応手列)
			search(pos, search_depth);

			// MultiPVで局面を足す、的な

			MoveListType move_list;

			int multi_pv = std::min((int)Options["MultiPV"], (int)th->rootMoves.size());
			for (int i = 0; i < multi_pv; ++i)
			{
				// 出現頻度は、バージョンナンバーを100倍したものにしておく)
				Move nextMove = (th->rootMoves[i].pv.size() >= 1) ? th->rootMoves[i].pv[1] : MOVE_NONE;
				BookPos bp(th->rootMoves[i].pv[0], nextMove, th->rootMoves[i].score
					, search_depth, int(atof(ENGINE_VERSION) * 100));

				// MultiPVで思考しているので、手番側から見て評価値の良い順に並んでいることは保証される。
				// (書き出しのときに並び替えなければ)
				move_list.push_back(bp);
			}

			{
				std::unique_lock<Mutex> lk(io_mutex);
				// 前のエントリーは上書きされる。
				BookEntry be(split_sfen(sfen), move_list);
				book.add(be, true);

				// 新たなエントリーを追加したのでフラグを立てておく。
				appended = true;
			}

			// 1局面思考するごとに'.'をひとつ出力する。
			if (progress_type == 0)
				std::cout << ".";
			else
				std::cout << ("info string progress " + std::to_string(get_loop_count()) + " / " + std::to_string(get_loop_max()) + "\n") << std::flush;
		}
	}
}
#endif

namespace BookUtil
{

#ifdef ENABLE_MAKEBOOK_CMD
	// ----------------------------------
	// USI拡張コマンド "bookutil"(定跡ツール)
	// ----------------------------------
	// フォーマット等についてはdoc/解説.txt を見ること。
	void bookutil_cmd(Position & pos, std::istringstream & is)
	{
		std::string token;
		is >> token;

		// sfenから生成する
		bool from_sfen = token == "from_sfen";
		// 自ら思考して生成する
		bool from_thinking = token == "think";
		// 定跡のマージ
		bool book_merge = token == "merge";
		bool book_hardfilter = token == "hardfilter";
		bool book_softfilter = token == "softfilter";
		// 定跡のsort
		bool book_sort = token == "sort";
		// 定跡からsfenを生成する
		bool to_sfen = token == "to_sfen";
		// 定跡から棋譜文字列を生成する
		bool to_kif1 = token == "to_kif1";
		bool to_kif2 = token == "to_kif2";
		bool to_csa1 = token == "to_csa1";
		// apery形式bookからの変換
		bool from_apery = token == "from_apery";

#if !defined(EVAL_LEARN) || !(defined(YANEURAOU_2017_EARLY_ENGINE) || defined(YANEURAOU_2017_GOKU_ENGINE))
		if (from_thinking)
		{
			std::cout << "Error!:define EVAL_LEARN and ( YANEURAOU_2017_EARLY_ENGINE or GOKU_ENGINE ) " << std::endl;
			return;
		}
#endif

		if (from_sfen || from_thinking)
		{
			// sfenファイル名
			is >> token;

			// 読み込むべきファイル名
			std::string sfen_file_name[COLOR_NB];

			// ここに "bw"(black and whiteの意味)と指定がある場合、
			// 先手局面用と後手局面用とのsfenファイルが異なるという意味。
			// つまり、このあとsfenファイル名の指定が2つ来ることを想定している。
			if (token == "bw")
			{
				is >> sfen_file_name[BLACK];
				is >> sfen_file_name[WHITE];
			}
			else {
				/*BLACKとWHITEと共通*/
				sfen_file_name[0] = token;
			}

			// 定跡ファイル名
			std::string book_name;
			is >> book_name;

			// 開始手数、終了手数、探索深さ
			int start_moves = 1;
			int moves = 16;
			int depth = 24;

			// 分散生成用。
			// 2/7なら、7個に分けて分散生成するときの2台目のPCの意味。
			// cluster 2 7
			// のように指定する。
			// 1/1なら、分散させない。
			// cluster 1 1
			// のように指定する。
			int cluster_id = 1;
			int cluster_num = 1;

			// 手番拘束
			bool blackonly = false;
			bool whiteonly = false;

			// 進捗表示形式 0:通常 1:冗長
			int progress_type = 0;

			while (true)
			{
				token = "";
				is >> token;
				if (token == "")
					break;
				if (token == "moves")
					is >> moves;
				else if (token == "depth")
					is >> depth;
				else if (from_thinking && token == "startmoves")
					is >> start_moves;
				else if (from_thinking && token == "cluster")
					is >> cluster_id >> cluster_num;
				else if (from_thinking && token == "blackonly")
					blackonly = true;
				else if (from_thinking && token == "whiteonly")
					whiteonly = true;
				else if (from_thinking && token == "progresstype")
					is >> progress_type;
				else
				{
					std::cout << "Error! : Illigal token = " << token << std::endl;
					return;
				}
			}

			if (from_sfen)
				std::cout << "read sfen moves " << moves << std::endl;
			if (from_thinking)
				std::cout << "read sfen moves from " << start_moves << " to " << moves
					<< " , depth = " << depth
					<< " , cluster = " << cluster_id << "/" << cluster_num << std::endl;

			std::cout << "..done" << std::endl;

			deqBook book;

			if (from_thinking)
			{
				std::cout << "read book.." << std::flush;

				// 初回はファイルがないので読み込みに失敗するが無視して続行。
				if (book.read_book(book_name) != 0)
					std::cout << "..but , create new file." << std::endl;
				else
					std::cout << "..done" << std::endl;
			}

			// この時点で評価関数を読み込まないとKPPTはPositionのset()が出来ないので…。
			is_ready();

			std::vector<std::string> sfens[COLOR_NB];
			for (auto c : COLOR)
				if (!sfen_file_name[c].empty())
					read_all_lines(sfen_file_name[c], sfens[c]);

			std::cout << "parse.." << std::flush;

			// 思考すべき局面のsfen
			std::unordered_set<std::string> thinking_sfens;

			// StateInfoのスタック
			Search::StateStackPtr ssp;

			// 各行の局面をparseして読み込む(このときに重複除去も行なう)
			for (auto c : COLOR)
			{
				bool enable_black = (!whiteonly && c == BLACK);
				bool enable_white = (!blackonly && (c == WHITE || sfen_file_name[1].empty()));
				for (size_t k = 0; k < sfens[c].size(); ++k)
				{
					auto sfen = std::move(sfens[c][k]);

					// "#"以降読み捨て
					size_t compos;
					if ((compos = sfen.find("#")) < sfen.length())
						sfen.resize(compos);

					if (sfen.length() == 0)
						continue;

					std::istringstream iss(sfen);
					fetch_initialpos(pos, iss);

					std::vector<Move> m;    // 初手から(moves+1)手までの指し手格納用
					std::vector<std::string> sf; // 初手から(moves+0)手までのsfen文字列格納用

					int plyinit = pos.game_ply();
					ssp = Search::StateStackPtr(new aligned_stack<StateInfo>);

					// sfenから直接生成するときはponderのためにmoves + 1の局面まで調べる必要がある。
					for (int i = plyinit; i <= moves + (from_sfen ? 1 : 0); ++i)
					{
						token = "";
						iss >> token;
						if (token == "")
						{
							// この局面、未知の局面なのでpushしないといけないのでは..
							if (!from_sfen)
								sf.push_back(pos.sfen());
							break;
						}

						Move move = move_from_usi(pos, token);
						// illigal moveであるとMOVE_NONEが返る。
						if (move == MOVE_NONE)
						{
							std::cout << "illegal move : line = " << (k + 1) << " , " << sfen << " , move = " << token << std::endl;
							break;
						}

						// MOVE_WIN,MOVE_RESIGNでは局面を進められないのでここで終了。
						if (!is_ok(move))
							break;
						
						sf.push_back(
							(~pos.side_to_move() ? enable_black : enable_white) ?
							pos.sfen() : "");
						m.push_back(move);

						ssp->push(StateInfo());
						if (move == MOVE_NULL)
							pos.do_null_move(ssp->top());
						else
							pos.do_move(move, ssp->top());
					}

					for (int i = 0; i < (int)sf.size() - (from_sfen ? 1 : 0); ++i)
					{
						if (i + plyinit < start_moves || sf[i] == "")
							continue;

						if (from_sfen)
						{
							// この場合、m[i + 1]が必要になるので、m.size()-1までしかループできない。
							BookPos bp(m[i], m[i + 1], VALUE_ZERO, 32, 1);
							auto ss = split_sfen(sf[i]);
							book.insert_book_pos(ss, bp);
						} else if (from_thinking)
						{
							// posの局面で思考させてみる。(あとでまとめて)
							if (thinking_sfens.count(sf[i]) == 0)
								thinking_sfens.insert(sf[i]);
						}
					}

					// sfenから生成するモードの場合、1000棋譜処理するごとにドットを出力。
					if ((k % 1000) == 0)
						std::cout << '.' << std::flush;
				}
			}
			// 入力が空でも初期局面は含めるように
			if (from_thinking && start_moves == 1 && thinking_sfens.empty())
			{
				pos.set_hirate();
				thinking_sfens.insert(pos.sfen());
			}
			std::cout << "done." << std::endl;

#if defined(EVAL_LEARN) && (defined(YANEURAOU_2017_EARLY_ENGINE) || defined(YANEURAOU_2017_GOKU_ENGINE))

			if (from_thinking)
			{
				// thinking_sfensを並列的に探索して思考する。
				// スレッド数(これは、USIのsetoptionで与えられる)
				u32 multi_pv = Options["MultiPV"];

				// 合法手チェック用
				Position pos_;

				// 思考する局面をsfensに突っ込んで、この局面数をg_loop_maxに代入しておき、この回数だけ思考する。
				MultiThinkBook multi_think(depth, book);

				// 進捗表示形式
				multi_think.progress_type = progress_type;

				auto & sfens_ = multi_think.sfens;
				for (auto & s : thinking_sfens)
				{

					// この局面のいま格納されているデータを比較して、この局面を再考すべきか判断する。
					auto it = book.find(trim_sfen(s));

					// deqBookにエントリーが存在しないなら無条件で、この局面について思考して良い。
					if (it == book.end())
						sfens_.push_back(s);
					else
					{
						auto & bp = it->move_list;
						if (bp.front().depth < depth || // 今回の探索depthのほうが深い
							(
								bp.front().depth == depth && bp.size() < multi_pv && // 探索深さは同じだが今回のMultiPVのほうが大きい
								bp.size() < MoveList<LEGAL>((pos_.set(s), pos_)).size() // かつ、合法手の数のほうが大きい
							)
						)
							sfens_.push_back(s);
					}
				}

#if 0
				// 思考対象局面が求まったので、sfenを表示させてみる。
				std::cout << "thinking sfen = " << std::endl;
				for (auto & s : sfens_)
					std::cout << "sfen " << s << std::endl;
#endif

				// 思考対象node数の出力。
				std::cout << "total " << sfens_.size() << " nodes " << std::endl;

				// クラスターの指定に従い、間引く。
				if (cluster_id != 1 || cluster_num != 1)
				{
					std::vector<std::string> a;
					for (int i = 0; i < (int)sfens_.size(); ++i)
					{
						if ((i % cluster_num) == cluster_id - 1)
							a.push_back(sfens_[i]);
					}
					sfens_ = a;

					// このPCに割り振られたnode数を表示する。
					std::cout << "for my PC : " << sfens_.size() << " nodes " << std::endl;
				}

				multi_think.set_loop_max(sfens_.size());

				// 30分ごとに保存
				// (ファイルが大きくなってくると保存の時間も馬鹿にならないのでこれくらいの間隔で妥協)
				multi_think.callback_seconds = 30 * 60;
				multi_think.callback_func = [&]()
				{
					std::unique_lock<Mutex> lk(multi_think.io_mutex);
					// 前回書き出し時からレコードが追加された？
					if (multi_think.appended)
					{
						book.write_book(book_name);
						std::cout << ("S" + std::to_string(multi_think.get_remain_loop_count()) + "\n") << std::flush;
						multi_think.appended = false;
					} else
						// 追加されていないときは小文字のsマークを表示して
						// ファイルへの書き出しは行わないように変更。
						std::cout << "s\n" << std::flush;

					// 置換表が同じ世代で埋め尽くされるとまずいのでこのタイミングで世代カウンターを足しておく。
					TT.new_search();
				};

				multi_think.go_think();

			}

#endif
			if (from_sfen)
			{
				std::cout << "write..";
				book.write_book(book_name);
				std::cout << "finished." << std::endl;
			}

		} else if (to_sfen || to_kif1 || to_kif2 || to_csa1)
		{
			// 定跡からsfenを生成する

			// 定跡ファイル名
			std::string book_name;
			is >> book_name;

			// sfenファイル名
			std::string sfen_name;
			is >> sfen_name;

			// 探索・出力オプション
			int moves = 256;
			int evalblackdiff = Options["BookEvalDiff"];
			int evalwhitediff = Options["BookEvalDiff"];
			int evalblacklimit = Options["BookEvalBlackLimit"];
			int evalwhitelimit = Options["BookEvalWhiteLimit"];
			int depthlimit = Options["BookDepthLimit"];
			int unregscore = -30000;
			bool comment = false;
			std::string opening = "";
			SquareFormat sqfmt = SqFmt_ASCII;

			while (true)
			{
				token = "";
				is >> token;
				if (token == "")
					break;
				if (token == "moves")
					is >> moves;
				else if (token == "evaldiff")
				{
					is >> evalblackdiff;
					evalwhitediff = evalblackdiff;
				}
				else if (token == "evalblackdiff")
					is >> evalblackdiff;
				else if (token == "evalwhitediff")
					is >> evalwhitediff;
				else if (token == "evallimit")
				{
					is >> evalblacklimit;
					evalwhitelimit = evalblacklimit;
				}
				else if (token == "evalblacklimit")
					is >> evalblacklimit;
				else if (token == "evalwhitelimit")
					is >> evalwhitelimit;
				else if (token == "unregscore")
					is >> unregscore;
				else if (token == "depthlimit")
					is >> depthlimit;
				else if (token == "squareformat")
					is >> sqfmt;
				else if (token == "opening")
					is >> quoted(opening);
				else if (token == "comment")
					comment = true;
				else
				{
					std::cout << "Error! : Illigal token = " << token << std::endl;
					return;
				}
			}

			std::cout << "read book.." << std::flush;
			deqBook book;
			if (book.read_book(book_name) != 0)
				std::cout << "..but , create kifs." << std::endl;
			std::cout << "..done. " << book.book_body.size() << " nodes loaded." << std::endl;

			is_ready();

			// カウンタ
			const u64 count_sfens_threshold = 10000;
			u64 count_sfens = 0, count_sfens_part = 0;

			std::vector<Move> m;
			std::vector<std::string> sf;	// sfen指し手文字列格納用

			auto SetupStates = Search::StateStackPtr(new aligned_stack<StateInfo>);
			std::vector<StateInfo> si(std::max(moves + 2, 258));

			std::fstream fs;
			fs.open(sfen_name, std::ios::out);

			std::string inisfen;

			std::cout << "export " << (to_sfen ? "sfens" : to_kif1 ? "kif1" : to_kif2 ? "kif2" : to_csa1 ? "csa1" : "") << " :"
				<< " moves " << moves
				<< " evalblackdiff " << evalblackdiff
				<< " evalwhitediff " << evalwhitediff
				<< " evalblacklimit " << evalblacklimit
				<< " evalwhitelimit " << evalwhitelimit
				<< " depthlimit " << depthlimit
				<< " opening " << quoted(opening)
				<< std::endl;

			// 探索結果種別
			enum BookRes
			{
				BOOKRES_UNFILLED,
				BOOKRES_SUCCESS,
				BOOKRES_DUPEPOS,
				BOOKRES_EMPTYLIST,
				BOOKRES_DEPTHLIMIT,
				BOOKRES_EVALLIMIT,
				BOOKRES_MOVELIMIT
			};

			// 文字列化
			auto to_movestr = [&](Position & _pos, Move _m, Move _m_back = MOVE_NONE)
			{
				return
					to_sfen ? to_usi_string(_m) :
					to_kif1 ? to_kif1_string(_m, _pos, _m_back, sqfmt) :
					to_kif2 ? to_kif2_string(_m, _pos, _m_back, sqfmt) :
					to_csa1 ? to_csa1_string(_m, _pos) :
					"";
			};

			// 結果出力
			auto printstack = [&](std::ostream & os, BookRes res)
			{
				if (to_sfen)
				{
					if (inisfen != SFEN_HIRATE)
						os << "sfen " << inisfen << "moves";
					else
						os << "startpos moves";
					for (auto & s : sf)
						os << " " << s;
				}
				else if (to_kif1 || to_kif2 || to_csa1)
				{
					if (inisfen != SFEN_HIRATE)
						os << "sfen " << inisfen << "moves ";
					for (auto & s : sf) { os << s; }
				}
				if (comment)
				{
					switch (res)
					{
					case BOOKRES_UNFILLED:   os << "#UNFILLED"; break;
					case BOOKRES_DUPEPOS:    os << "#DUPEPOS"; break;
					case BOOKRES_EMPTYLIST:  os << "#EMPTYLIST"; break;
					case BOOKRES_DEPTHLIMIT: os << "#DEPTHLIMIT"; break;
					case BOOKRES_EVALLIMIT:  os << "#EVALLIMIT"; break;
					case BOOKRES_MOVELIMIT:  os << "#MOVELIMIT"; break;
					default:;
					}
				}
				os << "\n";
				// 出力行数のカウントアップ
				++count_sfens;
				if (++count_sfens_part >= count_sfens_threshold)
				{
					std::cout << '.' << std::flush;
					count_sfens_part = 0;
				}
			};

			// 再帰探索
			std::function<BookRes()> to_sfen_func;
			to_sfen_func = [&]()
			{
				auto it = book.find(pos);
				// この局面が定跡登録されてなければ戻る。
				if (it == book.end())
					return BOOKRES_UNFILLED;
				auto & be = *it;
				// 探索済み局面(BookEntryのplyをフラグ代わり)なら戻る
				if (be.ply == 0)
					return BOOKRES_DUPEPOS;
				// move_listが空なら戻る
				if (be.move_list.empty())
					return BOOKRES_EMPTYLIST;
				// Move正規化
				move_update(pos, be.move_list);
				// 到達済み局面のフラグ立て
				be.ply = 0;
				// 最善手が駄目なら戻る
				auto & bp0 = be.move_list.front();
				int evaldiff = ~pos.side_to_move() ? evalblackdiff : evalwhitediff;
				int evallimit = ~pos.side_to_move() ? evalblacklimit : evalwhitelimit;
				if (bp0.depth < depthlimit)
					return BOOKRES_DEPTHLIMIT;
				if (bp0.value < evallimit)
					return BOOKRES_EVALLIMIT;
				// 最善手以外に何番目の候補手まで探索するか
				auto bpitend = next(be.move_list.cbegin());
				for (;
					bpitend != be.move_list.cend() &&
					bpitend->value >= evallimit &&
					bpitend->value + evaldiff >= bp0.value;
					++bpitend);
				// 先の局面を探索
				int ply = pos.game_ply();
				for (auto bpit = be.move_list.cbegin(); bpit != bpitend; ++bpit)
				{
					Move nowMove = bpit->bestMove;
					// 探索スタック積み
					sf.push_back(to_movestr(pos, nowMove, m.empty() ? MOVE_NONE : m.back()));
					m.push_back(nowMove);
					SetupStates->push(StateInfo());
					pos.do_move(nowMove, SetupStates->top());
					// 先の局面で駄目出しされたら、その局面までの手順を出力
					auto res = (ply >= moves) ? BOOKRES_MOVELIMIT : to_sfen_func();
					if (res != BOOKRES_SUCCESS)
						printstack(fs, res);
					// 探索スタック崩し
					pos.undo_move(nowMove);
					SetupStates->pop();
					m.pop_back();
					sf.pop_back();
				}
				return BOOKRES_SUCCESS;
			};

			// 探索開始
			std::istringstream ssop(opening);
			std::string opsfen;
			getline(ssop, opsfen, ',');
			do
			{
				sf.clear();
				m.clear();
				std::istringstream ssopsfen(opsfen);
				inisfen = fetch_initialpos(pos, ssopsfen);
				std::string movetoken;
				while (ssopsfen >> movetoken)
				{
					Move _mv = pos.move16_to_move(move_from_usi(movetoken));
					if (!(is_ok(_mv) && pos.pseudo_legal(_mv) && pos.legal(_mv)))
						break;
					sf.push_back(to_movestr(pos, _mv, m.empty() ? MOVE_NONE : m.back()));
					m.push_back(_mv);
					SetupStates->push(StateInfo());
					pos.do_move(_mv, SetupStates->top());
				}
				BookRes res = to_sfen_func();
				if (res != BOOKRES_SUCCESS)
					printstack(fs, res);
			} while (getline(ssop, opsfen, ','));

			// 探索終了
			fs.close();
			std::cout << ".finished!" << std::endl;
			std::cout << count_sfens << " lines exported." << std::endl;

		} else if (from_apery)
		{
			std::string book_name[2];
			is >> book_name[0] >> book_name[1];
			if (book_name[1] == "")
			{
				std::cout << "Error! book name is empty." << std::endl;
				return;
			}
			std::string opening = "";
			int unregDepth_default = 1;
			while (true)
			{
				token = "";
				is >> token;
				if (token == "")
					break;
				if (token == "opening")
					is >> quoted(opening);
				if (token == "unregdepth")
					is >> unregDepth_default;
				else
				{
					std::cout << "Error! : Illigal token = " << token << std::endl;
					return;
				}
			}

			is_ready();

			AperyBook apery_book(book_name[0]);
			std::cout << apery_book.apery_book.size() << " nodes loaded." << std::endl;
			deqBook work_book;
			const int cache_bitlen = 24;
			const int cache_idxmask = (1 << cache_bitlen) - 1;
			std::vector<std::vector<Book::Key>> cacheKey;
			for (int i = 0; i < unregDepth_default; ++i)
			{
				cacheKey.emplace_back(1 << cache_bitlen, static_cast<Book::Key>(0));
				cacheKey.back()[0] = static_cast<Book::Key>(-1LL);
			}
			u64 count_pos = 0;
			// 探索結果種別
			enum BookRes
			{
				BOOKRES_UNFILLED,
				BOOKRES_SUCCESS,
				BOOKRES_DUPEPOS
			};
			// 局面コピー
			std::function<BookRes(const Position &)> regPos = [&](const Position &)
			{
				auto mlist = apery_book.get_entries(pos);
				if (!mlist) { return BOOKRES_UNFILLED; }
				BookEntry be(pos, *mlist);
				if (work_book.add(be, true) == 1)
					return BOOKRES_DUPEPOS;
				++count_pos;
				if (count_pos % 1000 == 0)
					std::cout << "." << std::flush;
				return BOOKRES_SUCCESS;
			};

			// 再帰探索
			std::function<BookRes(int)> recSearch = [&](int unregDepth)
			{
				BookRes res = regPos(pos);
				int nextUnreg = 0;
				bool f = false;
				Book::Key bookKey;
				int cacheIdx;
				switch (res)
				{
				case BOOKRES_UNFILLED:
					if(unregDepth < 1)
						return res;
					nextUnreg = unregDepth - 1;
					bookKey = Book::AperyBook::bookKey(pos);
					cacheIdx = (static_cast<int>(bookKey) & cache_idxmask);
					for (int i = nextUnreg; i >= 0; --i)
						if (cacheKey[i][cacheIdx] == bookKey)
							return res;
					cacheKey[nextUnreg][cacheIdx] = bookKey;
					break;
				case BOOKRES_SUCCESS:
					nextUnreg = unregDepth_default;
					f = true;
					break;
				case BOOKRES_DUPEPOS:
					return res;
				}
				for (ExtMove em : MoveList<LEGAL_ALL>(pos))
				{
					StateInfo si;
					Move m = em.move;
					pos.do_move(m, si);
					if (recSearch(nextUnreg) == BOOKRES_SUCCESS)
						f = true;
					pos.undo_move(m);
				}
				return f ? BOOKRES_SUCCESS : BOOKRES_UNFILLED;
			};

			// 探索開始
			std::istringstream ssop(opening);
			std::string opsfen;
			getline(ssop, opsfen, ',');
			do
			{
				std::istringstream ssopsfen(opsfen);
				std::string inisfen = fetch_initialpos(pos, ssopsfen);

				std::string movetoken;
				while (ssopsfen >> movetoken)
				{
					regPos(pos);
					Move _mv = pos.move16_to_move(move_from_usi(movetoken));
					if (!(is_ok(_mv) && pos.pseudo_legal(_mv) && pos.legal(_mv)))
						break;
					StateInfo si;
					pos.do_move(_mv, si);
				}
				recSearch(unregDepth_default);
			} while (getline(ssop, opsfen, ','));

			// 探索終了
			apery_book.close_book();
			work_book.write_book(book_name[1]);
			auto count_nodes = work_book.book_body.size();
			work_book.close_book();
			std::cout << ".finished!" << std::endl;
			std::cout << count_nodes << " nodes exported." << std::endl;

		} else if (book_merge)
		{
			// 定跡のマージ
			deqBook book[3];
			std::string book_name[3];
			is >> book_name[0] >> book_name[1] >> book_name[2];
			if (book_name[2] == "")
			{
				std::cout << "Error! book name is empty." << std::endl;
				return;
			}
			std::cout << "book merge from " << book_name[0] << " and " << book_name[1] << " to " << book_name[2] << std::endl;
			for (int i = 0; i < 2; ++i)
			{
				std::cout << "reading book" << i << ".";
				if (book[i].read_book(book_name[i]) != 0)
					return;
				std::cout << ".done." << std::endl;
			}

			// 読み込めたので合体させる。
			std::cout << "merge..";

			// 同一nodeと非同一nodeの統計用
			// diffrent_nodes0 = book0側にのみあったnodeの数
			// diffrent_nodes1 = book1側にのみあったnodeの数
			u64 same_nodes = 0;
			u64 diffrent_nodes0 = 0, diffrent_nodes1 = 0;

			// マージ
			while (!book[0].book_body.empty() && !book[1].book_body.empty())
			{
				BookEntry & b0front = book[0].book_body.front();
				BookEntry & b1front = book[1].book_body.front();
				int cmp = b0front.compare(b1front);
				if (cmp == 0)
				{
					// 同じ局面があったので、良いほうをbook2に突っ込む。
					same_nodes++;
					BookEntry be = std::move(b0front);
					be.select(std::move(b1front));
					book[2].add(be);
					book[0].book_body.pop_front();
					book[1].book_body.pop_front();
				} else if (cmp < 0)
				{
					// book0からbook2に突っ込む
					diffrent_nodes0++;
					book[2].add(b0front);
					book[0].book_body.pop_front();
				} else
				{
					// book1からbook2に突っ込む
					diffrent_nodes1++;
					book[2].add(b1front);
					book[1].book_body.pop_front();
				}
			}
			// book0側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[0].book_body.empty())
			{
				diffrent_nodes0++;
				book[2].add(book[0].book_body.front());
				book[0].book_body.pop_front();
			}
			book[0].book_body.clear();
			// book1側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[1].book_body.empty())
			{
				diffrent_nodes1++;
				book[2].add(book[1].book_body.front());
				book[1].book_body.pop_front();
			}
			book[1].book_body.clear();
			std::cout << "..done" << std::endl;

			std::cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << std::endl;

			std::cout << "write..";
			book[2].write_book(book_name[2]);
			std::cout << "..done!" << std::endl;

		} else if (book_hardfilter || book_softfilter)
		{
			// 一致している定跡の抜き出し
			deqBook book[3];
			std::string book_name[3];
			is >> book_name[0] >> book_name[1] >> book_name[2];
			if (book_name[2] == "")
			{
				std::cout << "Error! book name is empty." << std::endl;
				return;
			}
			std::cout << "book filter " << book_name[0] << " with " << book_name[1] << " to " << book_name[2] << std::endl;
			for (int i = 0; i < 2; ++i)
			{
				std::cout << "reading book" << i << ".";
				if (book[i].read_book(book_name[i]) != 0)
					return;
				std::cout << ".done." << std::endl;
			}

			// 読み込めたので抽出する。
			std::cout << "filter..";

			// 同一nodeと非同一nodeの統計用
			// diffrent_nodes0 = book0側にのみあったnodeの数
			// diffrent_nodes1 = book1側にのみあったnodeの数
			u64 same_nodes = 0;
			u64 diffrent_nodes0 = 0, diffrent_nodes1 = 0;

			// フィルタ
			while (!book[0].book_body.empty() && !book[1].book_body.empty())
			{
				BookEntry & b0front = book[0].book_body.front();
				BookEntry & b1front = book[1].book_body.front();
				int cmp = b0front.compare(b1front);
				if (cmp == 0)
				{
					// 同じ局面があったので、book0からbook2に突っ込む。
					same_nodes++;
					BookEntry be0 = std::move(b0front);
					BookEntry be1 = std::move(b1front);
					book[0].book_body.pop_front();
					book[1].book_body.pop_front();
					if (book_hardfilter) {
						BookEntry be(be0.sfenPos, (be1.ply > 0 && (be0.ply > be1.ply || be0.ply < 1)) ? be1.ply : be0.ply);
						for (BookPos bp0 : be0.move_list)
							for (BookPos bp1 : be1.move_list)
								if (bp0.bestMove == bp1.bestMove)
								{
									be.move_list.push_back(bp0);
									break;
								}
						if (!be.move_list.empty())
							book[2].add(be);
					}
					else
						book[2].add(be0);
				}
				else if (cmp < 0)
				{
					// book0から読み捨てる
					diffrent_nodes0++;
					BookEntry be = std::move(b0front);
					book[0].book_body.pop_front();
				}
				else
				{
					// book1から読み捨てる
					diffrent_nodes1++;
					BookEntry be = std::move(b1front);
					book[1].book_body.pop_front();
				}
			}
			// book0側でまだ突っ込んでいないnodeを読み捨てる
			diffrent_nodes0 += book[0].book_body.size();
			book[0].book_body.clear();
			// book1側でまだ突っ込んでいないnodeを読み捨てる
			diffrent_nodes1 += book[1].book_body.size();
			book[1].book_body.clear();
			std::cout << "..done" << std::endl;

			std::cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << std::endl;

			std::cout << "write..";
			book[2].write_book(book_name[2]);
			std::cout << "..done!" << std::endl;

		} else if (book_sort)
		{
			// 定跡のsort, normalization(n11n)
			deqBook book;
			std::string book_src, book_dst;
			is >> book_src >> book_dst;
			is_ready();
			std::cout << "book sort from " << book_src << " , write to " << book_dst << std::endl;
			if (book.read_book(book_src, true) != 0)
				return;

			std::cout << "write..";
			book.write_book(book_dst);
			std::cout << "..done!" << std::endl;

		} else
		{
			std::cout << "usage" << std::endl;
			std::cout << "> bookutil from_sfen book.sfen book.db moves 24" << std::endl;
			std::cout << "> bookutil think book.sfen book.db moves 16 depth 18" << std::endl;
			std::cout << "> bookutil merge book_src1.db book_src2.db book_merged.db" << std::endl;
			std::cout << "> bookutil sort book_src.db book_sorted.db" << std::endl;
			std::cout << "> bookutil to_sfen book.db book.sfen moves 256 evaldiff 30 evalblacklimit 0 evalwhitelimit -140 depthlimit 16" << std::endl;
		}
	}
#endif

	// sfen文字列のゴミを除いた長さ
	size_t trimlen_sfen(const char * sfen, const size_t length)
	{
		auto cur = length;
		while (cur > 0)
		{
			char c = sfen[cur - 1];
			// 改行文字、スペース、数字(これはgame ply)ではないならループを抜ける。
			// これらの文字が出現しなくなるまで末尾を切り詰める。
			if (c != '\r' && c != '\n' && c != ' ' && !('0' <= c && c <= '9'))
				break;
			cur--;
		}
		return cur;
	}
	size_t trimlen_sfen(const std::string sfen)
	{
		return trimlen_sfen(sfen.c_str(), sfen.length());
	}

	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const std::string trim_sfen(const std::string sfen)
	{
		std::string s = sfen;
		s.resize(trimlen_sfen(sfen));
		return s;
	}

	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const std::string sfen)
	{
		auto cur = trimlen_sfen(sfen);
		std::string s = sfen;
		s.resize(cur);
		int ply = QConv::atos32(sfen.c_str() + cur);
		return make_pair(s, ply);
	}

	// 棋譜文字列ストリームから初期局面を抽出して設定
	std::string fetch_initialpos(Position & pos, std::istream & is)
	{
		std::string sfenpos, token;
		auto ispos = is.tellg();
		if (!(is >> token) || token == "moves")
			sfenpos = SFEN_HIRATE;
		else if (token != "position" && token != "sfen" && token != "startpos")
		{
			// 先頭句が予約語でないなら、いきなり平手からの指し手文字列が始まったと見なす
			sfenpos = SFEN_HIRATE;
			// シーク位置を戻す
			is.seekg(ispos);
		}
		else do
		{
			if (token == "moves")
				break;
			else if (token == "startpos")
				sfenpos = SFEN_HIRATE;
			else if (token == "sfen")
				sfenpos = "";
			else if (token != "position")
				sfenpos += token + " ";
		} while (is >> token);
		// 初期局面を設定
		pos.set(sfenpos);
		return sfenpos;
	}

	// Move 書き出し
	void movetoa(char ** s, const Move m)
	{
		char * _s = *s;
		if (!is_ok(m))
		{
			// 頻度が低いのでstringで手抜き
			std::string str;
			switch (m)
			{
			case MOVE_RESIGN:
				str = "resign";
				break;
			case MOVE_WIN:
				str = "win";
				break;
			case MOVE_NULL:
				str = "null";
				break;
			case MOVE_NONE:
				str = "none";
				break;
			default:;
			}
			size_t movelen = str.size();
			std::char_traits<char>::copy(_s, str.c_str(), movelen);
			_s += movelen;
		}
		else if (is_drop(m))
		{
			Square sq_to = move_to(m);
			*_s++ = USI_PIECE[(m >> 6) & 30]; // == USI_PIECE[(move_dropped_piece(m) & 15) * 2];
			*_s++ = '*';
			*_s++ = (char)('1' + file_of(sq_to));
			*_s++ = (char)('a' + rank_of(sq_to));
		}
		else
		{
			Square sq_from = move_from(m), sq_to = move_to(m);
			*_s++ = (char)('1' + file_of(sq_from));
			*_s++ = (char)('a' + rank_of(sq_from));
			*_s++ = (char)('1' + file_of(sq_to));
			*_s++ = (char)('a' + rank_of(sq_to));
			if (is_promote(m))
				*_s++ = '+';
		}
		*_s = '\0'; // 念のためNULL文字出力
		*s = _s;
	}

	// BookEntry 書き出し
	void betoa(char ** s, const BookEntry & be)
	{
		char * _s = *s;

		// 先頭の"sfen "と末尾の指し手手数を除いた sfen文字列 の長さは最大で 95bytes?
		// "+l+n+sgkg+s+n+l/1+r5+b1/+p+p+p+p+p+p+p+p+p/9/9/9/+P+P+P+P+P+P+P+P+P/1+B5+R1/+L+N+SGKG+S+N+L b -"
		// 128bytes を超えたら明らかにおかしいので抑止。
		*_s++ = 's';
		*_s++ = 'f';
		*_s++ = 'e';
		*_s++ = 'n';
		*_s++ = ' ';
		size_t sfenlen = std::min(be.sfenPos.size(), (size_t)128);
		std::char_traits<char>::copy(_s, be.sfenPos.c_str(), sfenlen);
		_s += sfenlen;
		*_s++ = ' ';
		QConv::s32toa(&_s, be.ply);
		*_s++ = '\n';
		// BookPosは改行文字を含めても 62bytes を超えないので、
		// be.move_list.size() <= 1000 なら64kiBの _buffer を食い尽くすことは無いはず
		// 1局面の合法手の最大は593（歩不成・2段香不成・飛不成・角不成も含んだ場合、以下局面例）なので、
		// sfen 8R/kSS1S1K2/4B4/9/9/9/9/9/3L1L1L1 b RB4GS4NL18P 1
		// be.move_list.size() > 1000 なら安全のため出力しない
		if (be.move_list.size() <= 1000)
			for (auto & bp : be.move_list)
			{
				movetoa(&_s, bp.bestMove);
				*_s++ = ' ';
				movetoa(&_s, bp.nextMove);
				*_s++ = ' ';
				QConv::s32toa(&_s, bp.value);
				*_s++ = ' ';
				QConv::s32toa(&_s, bp.depth);
				*_s++ = ' ';
				QConv::u64toa(&_s, bp.num);
				*_s++ = '\n';
			}
		*_s = '\0'; // 念のためNULL文字出力
		*s = _s;
	}

	// 文字列 → Move
	Move atomove(const char ** p)
	{
		const char * _p = *p;
		char c0 = *_p;
		if (c0 >= '1' && c0 <= '9')
		{
			char c1 = *++_p; if (c1 < 'a' || c1 > 'i') goto _atomove_none;
			char c2 = *++_p; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++_p; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			char c4 = *++_p;
			if (c4 == '+')
			{
				*p = _p + 1;
				return make_move_promote(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
			else
			{
				*p = _p;
				return make_move(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
		}
		else if (c0 >= 'A' && c0 <= 'Z')
		{
			char c1 = *++_p; if (c1 != '*') goto _atomove_none;
			char c2 = *++_p; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++_p; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			*p = _p + 1;
			for (int i = 1; i <= 7; ++i)
				if (PieceToCharBW[i] == c0)
					return make_move_drop((Piece)i, toFile(c2) | toRank(c3));
		}
		else if (c0 == '0' || (c0 >= 'a' && c0 <= 'z'))
		{
			if (!memcmp(_p, "win", 3)) { *p = _p + 3; return MOVE_WIN; }
			if (!memcmp(_p, "null", 4)) { *p = _p + 4; return MOVE_NULL; }
			if (!memcmp(_p, "pass", 4)) { *p = _p + 4; return MOVE_NULL; }
			if (!memcmp(_p, "0000", 4)) { *p = _p + 4; return MOVE_NULL; }
		}
	_atomove_none:;
		while (*_p >= '*') ++_p;
		*p = _p;
		return MOVE_NONE;
	}

	Move atomove(const char * p)
	{
		return atomove(&p);
	}
	
	int OnTheFlyBook::read_book(const std::string & filename)
	{
		if (fs.is_open())
			fs.close();

		fs.open(filename, std::ifstream::in);
		if (fs.fail())
		{
			std::cout << "info string Error! : can't read " + filename << std::endl;
			return 1;
		}

		on_the_fly = true;
		book_name = filename;
		return 0;
	}

	int OnTheFlyBook::close_book()
	{
		if (fs.is_open())
			fs.close();
		on_the_fly = false;
		book_name = "";
		return 0;
	}

	MoveListTypeOpt OnTheFlyBook::get_entries(const Position & pos)
	{
		if (!on_the_fly)
			return {};

		// 末尾の手数は取り除いておく。
		// read_book()で取り除くと、そのあと書き出すときに手数が消失するのでまずい。(気がする)
		auto sfen = pos.trimedsfen();

		// ファイル自体はオープンされてして、ファイルハンドルはfsだと仮定して良い。

		// ファイルサイズ取得
		// C++的には未定義動作だが、これのためにsys/stat.hをincludeしたくない。
		// ここでfs.clear()を呼ばないとeof()のあと、tellg()が失敗する。
		fs.clear();
		fs.seekg(0, std::ios::end);
		auto file_end = fs.tellg();

		fs.clear();
		fs.seekg(0, std::ios::beg);
		auto file_start = fs.tellg();

		auto file_size = u64(file_end - file_start);

		// 与えられたseek位置から"sfen"文字列を探し、それを返す。どこまでもなければ""が返る。
		// hackとして、seek位置は-80しておく。
		auto next_sfen = [&](u64 seek_from)
		{
			std::string line;

			fs.seekg(std::max(s64(0), (s64)seek_from - 80), std::fstream::beg);
			getline(fs, line); // 1行読み捨てる

							   // getlineはeof()を正しく反映させないのでgetline()の返し値を用いる必要がある。
			while (getline(fs, line))
				// "sfen "という文字列は取り除いたものを返す。
				// 手数の表記も取り除いて比較したほうがいい。
				// ios::binaryつけているので末尾に'\R'が付与されている。禿げそう。
				if (!line.compare(0, 5, "sfen "))
					return trim_sfen(line.substr(5));
			return std::string();
		};

		// バイナリサーチ

		u64 s = 0, e = file_size, m;

		while (true)
		{
			m = (s + e) / 2;

			auto sfen2 = next_sfen(m);
			if (sfen2 == "" || sfen < sfen2)
				// 左(それより小さいところ)を探す
				e = m;
			else if (sfen > sfen2)
				// 右(それより大きいところ)を探す
				s = u64(fs.tellg() - file_start);
			else
				// 見つかった！
				break;

			// 40バイトより小さなsfenはありえないので探索範囲がこれより小さいなら終了。
			// ただしs = 0のままだと先頭が探索されていないので..
			// s,eは無符号型であることに注意。if (s-40 < e) と書くとs-40がマイナスになりかねない。
			if (s + 40 > e)
			{
				if (s != 0 || e != 0)
					// 見つからなかった
					return {};

				// もしかしたら先頭付近にあるかも知れん..
				e = 0; // この条件で再度サーチ
			}

		}
		// 見つけた処理

		// 行バッファ確保
		const size_t _buffersize = 256;
		char _buffer[_buffersize];

		BookEntry be(sfen, pos.game_ply());
		be.incpos(fs, _buffer, _buffersize);
		be.sort_pos();
		move_update(pos, be.move_list);
		return be.move_list;
	}

	// 定跡ファイルの読み込み(book.db)など。
	int deqBook::read_book(const std::string & filename)
	{
		return read_book(filename, false);
	}
	int deqBook::read_book(const std::string & filename, bool sfen_n11n)
	{
		// 読み込み済であるかの判定
		if (book_name == filename)
			return 0;

		// 別のファイルを開こうとしているなら前回メモリに丸読みした定跡をクリアしておかないといけない。
		if (book_name != "")
			clear();

		// 1行ごとの読み込みはオーバーヘッドが大きいので、作業領域を介して読み込む

		// ファイルハンドラ
		std::ifstream fs;
		
		// 読み込みバッファ
		const size_t buffersize = 16 * 1024 * 1024; // 16MiB
		char * buffer = new char[buffersize];
		// 読み込み範囲始点、読み込み範囲終点、バッファ内最終行始点、バッファ終点
		size_t idx0 = 0, idx1 = 0, lastheadidx = 0, endidx = 0;

		// バッファへの次ブロック読み込み
		auto readblock = [&]()
		{
			size_t _idx = 0, _remain = 0;
			// 読み残し領域を先頭にコピー
			if (idx0 < endidx)
				memcpy(buffer, buffer + idx0, _idx = endidx - idx0);
			idx0 = idx1 = 0;
			// バッファの残りサイズ
			_remain = buffersize - _idx;
			// 1文字分の余裕を残して読み込み、空いた1文字はNULで埋める
			endidx = _idx + (_remain > 1 ? (size_t)(fs.read(buffer + _idx, _remain - 1).gcount()) : 0);
			char * _p = buffer + endidx;
			*_p = '\0';
			// 最終行の行頭位置検出
			size_t _lastheadidx = endidx;
			for (; _lastheadidx > 0 && *--_p != '\n'; --_lastheadidx);
			// 全く改行文字が存在しなければ、行末まで読み捨てる
			if (_lastheadidx == 0 && *buffer != '\n')
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			lastheadidx = _lastheadidx;
		};

		// バッファ内の次行範囲探索、必要に応じてバッファへの次ブロック読み込み
		auto nextrange = [&]()
		{
			// 前回の終端を始点として次の行頭位置を調べる
			size_t _idx0 = idx1, _lastheadp0 = lastheadidx;
			char * _p0 = buffer + _idx0;
			while (_idx0 < _lastheadp0 && *_p0++ == '\n') ++_idx0;
			idx0 = _idx0;
			// バッファを読み終わるなら次ブロックをバッファに読み込み
			if (_idx0 >= _lastheadp0)
				readblock();
			// 行末位置を調べる
			size_t _idx1 = idx0, _lastheadp1 = lastheadidx;
			char * _p1 = buffer + _idx1;
			while (_idx1 < _lastheadp1 && *_p1++ != '\n') ++_idx1;
			idx1 = _idx1;
		};

		fs.open(filename, std::ifstream::in | std::ifstream::binary);
		if (fs.bad() || !fs.is_open())
		{
			std::cout << "info string Error! : can't read " + filename << std::endl;
			return 1;
		}

		// 先頭行読み込み
		nextrange();
		while (idx1 > 0)
		{
			// "sfen " で始まる25バイト以上の行を見つける
			if (memcmp(buffer + idx0, "sfen ", 5) || idx0 + 24 > idx1)
			{
				nextrange();
				continue;
			}
			// BookEntry 構築
			BookEntry be(buffer + idx0 + 5, idx1 - idx0 - 5, sfen_n11n);
			// BookEntry への BookPos要素充填
			while (true)
			{
				nextrange();
				// ファイル終端に達したら終了
				if (idx1 == 0)
					break;
				// 次の sfen が始まったら終了
				if (!memcmp(buffer + idx0, "sfen ", 5))
					break;
				// 行頭文字がおかしければスキップして次の行を調べる
				char c = buffer[idx0];
				if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
					continue;
				// BookEntry に BookPos 構築 & 追加
				be.move_list.emplace_back(buffer + idx0);
			}
			// おかしければ登録せずに次へ
			if (be.sfenPos.empty() || be.move_list.empty())
				continue;
			// 採択回数でsortしておく
			// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
			be.sort_pos();
			// dMemoryBook に登録
			add(be);
		}

		fs.close();

		delete[] buffer;

		// 全体のソート、重複局面の除去
		intl_uniq();

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book_name = filename;

		return 0;
	}

	// 定跡ファイルの書き出し
	int deqBook::write_book(const std::string & filename)
	{
		// 全体のソートと重複局面の除去
		intl_uniq();

		// 1行ごとの書き出しはオーバーヘッドが大きいので、作業領域を介して書き出す

		// 1要素あたりの最大サイズ
		const size_t entrymaxsize = 65536; // 64kiB
		// 領域確保係数
		const size_t renderbufmulti = 256;
		// 最大バッファ領域サイズ
		const size_t renderbufmaxsize = entrymaxsize * renderbufmulti; // 16MiB
		// バッファ領域サイズ
		size_t renderbufsize = entrymaxsize * std::min(renderbufmulti, book_body.size() + 1);
		// バッファ払い出し閾値
		ptrdiff_t renderbufth = (ptrdiff_t)renderbufsize - (ptrdiff_t)entrymaxsize;
		// バッファ確保
		char * renderbuf = new char[renderbufsize];

		// 文字列バッファ char[] に BookEntry を書き出し
		// BookEntry 毎に書き出しを行う
		char * p;

		std::ofstream fs;
		fs.open(filename, std::ofstream::out);
		if (fs.bad() || !fs.is_open())
		{
			std::cout << "info string Error! : can't write " + filename << std::endl;
			return 1;
		}

		// バージョン識別用文字列
		fs << "#YANEURAOU-DB2016 1.00" << std::endl;

		*(p = renderbuf) = '\0';
		for (BookEntry & be : book_body)
		{
			// 指し手のない空っぽのentryは書き出さないように。
			if (be.move_list.empty()) continue;
			// 採択回数でソートしておく。
			be.sort_pos();
			// 出力
			betoa(&p, be);
			// _bufth (16320kiB) を超えて出力が溜まったらファイルに書き出す
			if (p - renderbuf > renderbufth)
			{
				fs.write(renderbuf, p - renderbuf);
				*(p = renderbuf) = '\0';
			}
		}
		// 残りの出力をファイルに書き出し
		if (p != renderbuf)
			fs.write(renderbuf, p - renderbuf);

		fs.close();

		// バッファ開放
		delete[] renderbuf;

		return 0;
	}

	int deqBook::close_book()
	{
		clear();
		return 0;
	}

	// 部分的マージソート（下記[1][2]が満たされるまで最後の2つの区間をマージソートする）
	// [1] ソート済み区間が2つ以上ある時、最後の区間は最後から2つ目の区間より小さい
	// [2] ソート済み区間が3つ以上ある時、(最後の区間+最後から2つ目の区間)は最後から3つ目の区間より小さい
	void deqBook::intl_merge_part()
	{
		bool f = !book_run.empty();
		auto end_d = distance(book_body.begin(), book_body.end());
		while (f)
		{
			f = false;
			auto run_size = book_run.size();
			BookIterDiff run_back2, run_back;
			BookType::iterator body_begin;
			switch (run_size)
			{
			case 1:
				run_back = book_run.back();
				if (run_back <= end_d - run_back)
				{
					body_begin = book_body.begin();
					inplace_merge(body_begin, next(body_begin, run_back), book_body.end());
					book_run.pop_back();
				}
				break;
			case 2:
				run_back2 = book_run.front();
				run_back = book_run.back();
				if (run_back - run_back2 <= end_d - run_back)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					f = true;
				}
				else if (run_back2 <= end_d - run_back2)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					body_begin = book_body.begin();
					inplace_merge(body_begin, next(body_begin, run_back2), book_body.end());
					book_run.pop_back();
				}
				break;
			default:
				run_back2 = book_run[run_size - 2];
				run_back = book_run.back();
				if (
					run_back - run_back2 <= end_d - run_back ||
					run_back2 - book_run[run_size - 3] <= end_d - run_back2
					)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					f = true;
				}
			}
		}
	}

	// マージソート(全体)
	void deqBook::intl_merge()
	{
		while (!book_run.empty())
		{
			BookIterDiff d1 = book_run.back();
			book_run.pop_back();
			BookIterDiff d0 = book_run.empty() ? (BookIterDiff)0 : book_run.back();
			auto body_begin = book_body.begin();
			inplace_merge(next(body_begin, d0), next(body_begin, d1), book_body.end());
		}
	}

	// 同一局面の整理
	// 大量に局面登録する際、毎度重複チェックを行うよりまとめて処理する方が？
	void deqBook::intl_uniq()
	{
		// あらかじめ全体をソートしておく
		intl_merge();

		if (book_body.empty()) return;
		BookType::iterator it0 = book_body.begin(), it1 = next(it0), end = book_body.end();
		while (it1 != end)
			if (*it0 == *it1)
			{
				do
					(*it0).select(std::move(*it1++));
				while (it1 != end && *it0 == *it1);
				it0 = book_body.erase(++it0, it1);
				end = book_body.end();
				it1 = next(it0);
			}
			else
			{
				++it0;
				++it1;
			}
	}

	// 局面の追加
	// 大量の局面を追加する場合、重複チェックを逐一は行わず(dofind_false)に、後で intl_uniq() を行うことを推奨
	// 返り値 0:新規局面追加 1:登録済み局面有り
	int deqBook::add(BookEntry & be, bool dofind)
	{
		BookType::iterator it;
		int cmp;
		if (dofind && (it = find(be)) != end())
		{
			// 同一局面が存在したら整理して再登録
			(*it).select(be);
			return 1;
		}
		else if (book_body.empty() || (cmp = be.compare(book_body.back())) > 0)
			// 順序関係が保たれているなら単純に末尾に追加
			book_body.push_back(be);
		else if (cmp == 0)
		{
			// 末尾と同一局面なら整理
			book_body.back().select(be);
			return 1;
		}
		else if (book_run.empty())
			if (book_body.size() < MINRUN)
				// 短ければ、挿入ソート
				book_body.insert(upper_bound(book_body.begin(), book_body.end(), be), be);
			else
			{
				// 長ければ、新しいソート済み範囲を追加
				book_run.push_back(distance(book_body.begin(), book_body.end()));
				book_body.push_back(be);
			}
		else
		{
			if (distance(next(book_body.begin(), book_run.back()), book_body.end()) < (BookIterDiff)MINRUN)
				// 短ければ、挿入ソート
				book_body.insert(upper_bound(next(book_body.begin(), book_run.back()), book_body.end(), be), be);
			else
			{
				// 長ければ、部分的なマージソートを試みてから
				intl_merge_part();
				// 新しいソート済み範囲を追加
				book_run.push_back(distance(book_body.begin(), book_body.end()));
				book_body.push_back(be);
			}
		}
		return 0;
	}

	// char文字列 の BookPos 化
	void BookEntry::init(const char * sfen, const size_t length, const bool sfen_n11n)
	{
		if(sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。		
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)		
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(std::string(sfen, length));
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		} else
		{
			auto cur = trimlen_sfen(sfen, length);
			sfenPos = std::string(sfen, cur);
			ply = QConv::atos32(sfen + cur);
		}
	}

	// ストリームからの BookEntry 読み込み
	void BookEntry::init(std::istream & is, char * , const size_t , const bool sfen_n11n)
	{
		std::string line;

		do
		{
			int c;
			// 行頭が's'になるまで読み飛ばす
			while ((c = is.peek()) != 's')
				if (c == EOF || !is.ignore(std::numeric_limits<std::streamsize>::max(), '\n'))
					return;
			// 行読み込み
			if (!getline(is, line))
				return;
			// "sfen "で始まる行は局面のデータであり、sfen文字列が格納されている。
			// 短すぎたり、sfenで始まらなかったりした行ならばスキップ
		} while (line.length() < 24 || line.compare(0, 5, "sfen ") != 0);
		if (sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。		
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)		
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(line.substr(5));
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		} else
		{
			auto sfen = split_sfen(line.substr(5));
			sfenPos = sfen.first;
			ply = sfen.second;
		}
	}

	// ストリームから BookEntry への BookPos 順次読み込み
	void BookEntry::incpos(std::istream & is, char * _buffer, const size_t _buffersize)
	{
		while (true)
		{
			int c;
			// 行頭が数字か英文字以外なら行末文字まで読み飛ばす
			while ((c = is.peek(), (c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')))
				if (c == EOF || !is.ignore(std::numeric_limits<std::streamsize>::max(), '\n'))
					return;
			// 次のsfen文字列に到達していそうなら離脱（指し手文字列で's'から始まるものは無い）
			if (c == 's')
				return;
			// 行読み込み
			is.getline(_buffer, _buffersize - 8);
			// バッファから溢れたら行末まで読み捨て
			if (is.fail())
			{
				is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				is.clear(is.rdstate() & ~std::ios_base::failbit);
			}
			// BookPos 読み込み
			move_list.emplace_back(_buffer);
		}
	}

	// バイト文字列からの BookPos 読み込み
	void BookPos::init(const char * p)
	{
		// 解析実行
		// 0x00 - 0x1f, 0x21 - 0x29, 0x80 - 0xff の文字が現れ次第中止
		// 特に、NULL, CR, LF 文字に反応する事を企図。TAB 文字でも中止。SP 文字連続でも中止。
		if (*p < '*') return;
		bestMove = atomove(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		nextMove = atomove(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		value = QConv::atos32(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		depth = QConv::atos32(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		num = QConv::atou64(&p);
	}

	// 出力ストリーム
	// write_book では最適化のため現在使われていない
	std::ostream & operator << (std::ostream & os, const BookPos & bp)
	{
		os << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << ' ' << bp.depth << ' ' << bp.num << "\n";
		return os;
	}
	std::ostream & operator << (std::ostream & os, const BookEntry & be)
	{
		os << "sfen " << be.sfenPos << " " << be.ply << "\n";
		for (auto & bp : be.move_list)
			os << bp;
		return os;
	}

}
