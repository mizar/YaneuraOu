#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <inttypes.h>

#include "../../shogi.h"
#include "book.h"
#include "book_util.h"
#include "../../position.h"
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
			Learner::search(pos, search_depth);

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
		bool to_csa = token == "to_csa";
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
				else if (token == "blackonly")
					blackonly = true;
				else if (token == "whiteonly")
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

		}
#ifdef USE_KIF_CONVERT_TOOLS
		else if (to_sfen || to_kif1 || to_kif2 || to_csa1 || to_csa)
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
			int colorfmt = (int)KifConvertTools::ColorFmt_KIF, sqfmt = (int)KifConvertTools::SqFmt_ASCII, sameposfmt = (int)KifConvertTools::SamePosFmt_Short;

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
				else if (token == "colorformat")
					is >> colorfmt;
				else if (token == "squareformat")
					is >> sqfmt;
				else if (token == "sameposformat")
					is >> sameposfmt;
				else if (token == "opening")
					is >> quoted(opening);
				else if (token == "opening_ply2all")
					opening = OPENING_PLY2ALL;
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

			KifConvertTools::KifFormat fmt(
				(KifConvertTools::ColorFormat)colorfmt,
				(KifConvertTools::SquareFormat)sqfmt,
				(KifConvertTools::SamePosFormat)sameposfmt
				);

			// 文字列化
			auto to_movestr = [&](Position & _pos, Move _m)
			{
				return
					to_sfen ? to_usi_string(_m) :
					to_kif1 ? KifConvertTools::to_kif_u8string(_pos, _m, fmt) :
					to_kif2 ? KifConvertTools::to_kif2_u8string(_pos, _m, fmt) :
					to_csa1 ? KifConvertTools::to_csa_string(_pos, _m, KifConvertTools::Csa1Fmt) :
					to_csa ? KifConvertTools::to_csa_string(_pos, _m, KifConvertTools::CsaFmt) :
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
					sf.push_back(to_movestr(pos, nowMove));
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
					sf.push_back(to_movestr(pos, _mv));
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

		}
#endif // #ifdef USE_KIF_CONVERT_TOOLS
		else if (from_apery)
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

}
