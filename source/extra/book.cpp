#include "../shogi.h"

#include <fstream>
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <inttypes.h>

#include "book.h"
#include "../position.h"
#include "../misc.h"
#include "../search.h"
#include "../thread.h"
#include "../learn/multi_think.h"
#include "../tt.h"

using namespace std;
using std::cout;

void is_ready();

namespace Book
{
#ifdef ENABLE_MAKEBOOK_CMD
	// ----------------------------------
	// USI拡張コマンド "makebook"(定跡作成)
	// ----------------------------------

	// 局面を与えて、その局面で思考させるために、やねうら王2016Midが必要。
#if defined(EVAL_LEARN) && (defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE) || defined(YANEURAOU_2017_EARLY_ENGINE))
	struct MultiThinkBook: public MultiThink
	{
		MultiThinkBook(int search_depth_, MemoryBook & book_)
			: search_depth(search_depth_), book(book_), appended(false) {}

		virtual void thread_worker(size_t thread_id);

		// 定跡を作るために思考する局面
		vector<string> sfens;

		// 定跡を作るときの通常探索の探索深さ
		int search_depth;

		// メモリ上の定跡ファイル(ここに追加していく)
		MemoryBook& book;

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
			cout << "info string progress " << get_loop_count() << " / " << get_loop_max() << endl;
		while ((id = get_next_loop_count()) != UINT64_MAX)
		{
			auto sfen = sfens[id];

			auto& pos = Threads[thread_id]->rootPos;
			pos.set(sfen);
			auto th = Threads[thread_id];
			pos.set_this_thread(th);

			if (pos.is_mated())
				continue;

			// depth手読みの評価値とPV(最善応手列)
			search(pos, search_depth);

			// MultiPVで局面を足す、的な

			vector<BookPos> move_list;

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
				cout << "." << flush;
			else
				cout << ("info string progress " + to_string(get_loop_count()) + " / " + to_string(get_loop_max()) + "\n") << flush;
		}
	}
#endif

	// 棋譜文字列ストリームから初期局面を抽出して設定
	string fetch_initialpos(Position& pos, istream& is)
	{
		string sfenpos, token;
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

	// フォーマット等についてはdoc/解説.txt を見ること。
	void makebook_cmd(Position& pos, istringstream& is)
	{
		string token;
		is >> token;

		// sfenから生成する
		bool from_sfen = token == "from_sfen";
		// 自ら思考して生成する
		bool from_thinking = token == "think";
		// 定跡のマージ
		bool book_merge = token == "merge";
		// 定跡のsort
		bool book_sort = token == "sort";
		// 定跡からsfenを生成する
		bool to_sfen = token == "to_sfen";
		// 定跡から棋譜文字列を生成する
		bool to_kif1 = token == "to_kif1";
		bool to_kif2 = token == "to_kif2";
		bool to_csa1 = token == "to_csa1";

#if !defined(EVAL_LEARN) || !(defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE) || defined(YANEURAOU_2017_EARLY_ENGINE))
		if (from_thinking)
		{
			cout << "Error!:define EVAL_LEARN and ( YANEURAOU_2016_MID_ENGINE or LATE_ENGINE or 2017_EARLY_ENGINE) " << endl;
			return;
		}
#endif

		if (from_sfen || from_thinking)
		{
			// sfenファイル名
			is >> token;
			string sfen_name = token;

			// 定跡ファイル名
			string book_name;
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
			bool disableblack = false;
			bool disablewhite = false;

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
					disablewhite = true;
				else if (from_thinking && token == "whiteonly")
					disableblack = true;
				else if (from_thinking && token == "progresstype")
					is >> progress_type;
				else
				{
					cout << "Error! : Illigal token = " << token << endl;
					return;
				}
			}

			if (from_sfen)
				cout << "read sfen moves " << moves << endl;
			if (from_thinking)
				cout << "read sfen moves from " << start_moves << " to " << moves
					<< " , depth = " << depth
					<< " , cluster = " << cluster_id << "/" << cluster_num << endl;

			cout << "..done" << endl;

			MemoryBook book;

			if (from_thinking)
			{
				cout << "read book.." << flush;

				// 初回はファイルがないので読み込みに失敗するが無視して続行。
				if (read_book(book_name, book) != 0)
					cout << "..but , create new file." << endl;
				else
					cout << "..done" << endl;
			}

			// この時点で評価関数を読み込まないとKPPTはPositionのset()が出来ないので…。
			is_ready();

			std::deque<string> sfens;
			read_all_lines(sfen_name, sfens);

			cout << "parse.." << flush;

			// 思考すべき局面のsfen
			unordered_set<string> thinking_sfens;

			// StateInfoのスタック
			Search::StateStackPtr ssp;

			// 各行の局面をparseして読み込む(このときに重複除去も行なう)
			for (size_t k = 1; !sfens.empty(); ++k)
			{
				auto sfen = std::move(sfens.front());
				sfens.pop_front();

				// "#"以降読み捨て
				size_t compos;
				if ((compos = sfen.find("#")) < sfen.length())
					sfen.resize(compos);

				if (sfen.length() == 0)
					continue;

				istringstream iss(sfen);
				fetch_initialpos(pos, iss);

				vector<Move> m;    // 初手から(moves+1)手までの指し手格納用
				vector<string> sf; // 初手から(moves+0)手までのsfen文字列格納用

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
						cout << "illegal move : line = " << k << " , " << sfen << " , move = " << token << endl;
						break;
					}

					// MOVE_WIN,MOVE_RESIGNでは局面を進められないのでここで終了。
					if (!is_ok(move))
						break;
					
					if (~pos.side_to_move() ? disableblack : disablewhite)
						sf.push_back("");
					else
						sf.push_back(pos.sfen());
					m.push_back(move);

					ssp->push(StateInfo());
					if(move == MOVE_NULL)
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
						book.insert_book_pos(sf[i], bp);
					} else if (from_thinking)
					{
						// posの局面で思考させてみる。(あとでまとめて)
						if (thinking_sfens.count(sf[i]) == 0)
							thinking_sfens.insert(sf[i]);
					}
				}

				// sfenから生成するモードの場合、1000棋譜処理するごとにドットを出力。
				if ((k % 1000) == 0)
					cout << '.' << flush;
			}
			// 入力が空でも初期局面は含めるように
			if (from_thinking && start_moves == 1 && thinking_sfens.empty())
			{
				pos.set_hirate();
				thinking_sfens.insert(pos.sfen());
			}
			cout << "done." << endl;

#if defined(EVAL_LEARN) && (defined(YANEURAOU_2016_MID_ENGINE)||defined(YANEURAOU_2016_LATE_ENGINE)||defined(YANEURAOU_2017_EARLY_ENGINE))

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

				auto& sfens_ = multi_think.sfens;
				for (auto& s : thinking_sfens)
				{

					// この局面のいま格納されているデータを比較して、この局面を再考すべきか判断する。
					auto it = book.intl_find(s);

					// MemoryBookにエントリーが存在しないなら無条件で、この局面について思考して良い。
					if (it == book.end())
						sfens_.push_back(s);
					else
					{
						auto& bp = it->move_list;
						if (bp[0].depth < depth || // 今回の探索depthのほうが深い
							(
								bp[0].depth == depth && bp.size() < multi_pv && // 探索深さは同じだが今回のMultiPVのほうが大きい
								bp.size() < MoveList<LEGAL>((pos_.set(s), pos_)).size() // かつ、合法手の数のほうが大きい
							)
						)
							sfens_.push_back(s);
					}
				}

#if 0
				// 思考対象局面が求まったので、sfenを表示させてみる。
				cout << "thinking sfen = " << endl;
				for (auto& s : sfens_)
					cout << "sfen " << s << endl;
#endif

				// 思考対象node数の出力。
				cout << "total " << sfens_.size() << " nodes " << endl;

				// クラスターの指定に従い、間引く。
				if (cluster_id != 1 || cluster_num != 1)
				{
					vector<string> a;
					for (int i = 0; i < (int)sfens_.size(); ++i)
					{
						if ((i % cluster_num) == cluster_id - 1)
							a.push_back(sfens_[i]);
					}
					sfens_ = a;

					// このPCに割り振られたnode数を表示する。
					cout << "for my PC : " << sfens_.size() << " nodes " << endl;
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
						write_book(book_name, book);
						cout << ("S" + to_string(multi_think.get_remain_loop_count()) + "\n") << flush;
						multi_think.appended = false;
					} else
					{
						// 追加されていないときは小文字のsマークを表示して
						// ファイルへの書き出しは行わないように変更。
						cout << "s\n" << flush;
					}

					// 置換表が同じ世代で埋め尽くされるとまずいのでこのタイミングで世代カウンターを足しておく。
					TT.new_search();
				};

				multi_think.go_think();

			}

#endif
			if (from_sfen)
			{
				cout << "write..";
				write_book(book_name, book);
				cout << "finished." << endl;
			}

		} else if (to_sfen || to_kif1 || to_kif2 || to_csa1)
		{
			// 定跡からsfenを生成する

			// 定跡ファイル名
			string book_name;
			is >> book_name;

			// sfenファイル名
			string sfen_name;
			is >> sfen_name;
			
			// 探索・出力オプション
			int moves = 256;
			int evalblackdiff = Options["BookEvalDiff"];
			int evalwhitediff = Options["BookEvalDiff"];
			int evalblacklimit = Options["BookEvalBlackLimit"];
			int evalwhitelimit = Options["BookEvalWhiteLimit"];
			int depthlimit = Options["BookDepthLimit"];
			bool comment = false;
			string opening = "";
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
					cout << "Error! : Illigal token = " << token << endl;
					return;
				}
			}

			cout << "read book.." << flush;
			MemoryBook book;
			if (read_book(book_name, book) != 0)
				cout << "..but , create kifs." << endl;
			cout << "..done" << endl;

			is_ready();

			// カウンタ
			const u64 count_sfens_threshold = 10000;
			u64 count_sfens = 0, count_sfens_part = 0;

			vector<Move> m;
			vector<string> sf;	// sfen指し手文字列格納用

			StateInfo si[258];

			fstream fs;
			fs.open(sfen_name, ios::out);

			string inisfen;

			cout << "export " << (to_sfen ? "sfens" : to_kif1 ? "kif1" : to_kif2 ? "kif2" : to_csa1 ? "csa1" : "") << " :"
				<< " moves " << moves
				<< " evalblackdiff " << evalblackdiff
				<< " evalwhitediff " << evalwhitediff
				<< " evalblacklimit " << evalblacklimit
				<< " evalwhitelimit " << evalwhitelimit
				<< " depthlimit " << depthlimit
				<< " opening " << quoted(opening)
				<< endl;

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
			auto to_movestr = [&](Position& _pos, Move _m, Move _m_back = MOVE_NONE)
			{
				return
					to_sfen ? to_usi_string(_m) :
					to_kif1 ? to_kif1_string(_m, _pos, _m_back, sqfmt) :
					to_kif2 ? to_kif2_string(_m, _pos, _m_back, sqfmt) :
					to_csa1 ? to_csa1_string(_m, _pos) :
					"";
			};

			// 結果出力
			auto printstack = [&](ostream& os, BookRes res)
			{
				if (to_sfen)
				{
					if (inisfen != SFEN_HIRATE)
						os << "sfen " << inisfen << "moves";
					else
						os << "startpos moves";
					for (auto& s : sf)
						os << " " << s;
				}
				else if (to_kif1 || to_kif2 || to_csa1)
				{
					if (inisfen != SFEN_HIRATE)
						os << "sfen " << inisfen << "moves ";
					for (auto& s : sf) { os << s; }
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
					}
				}
				os << endl;
				// 出力行数のカウントアップ
				++count_sfens;
				if (++count_sfens_part >= count_sfens_threshold)
				{
					cout << '.' << flush;
					count_sfens_part = 0;
				}
			};

			// 再帰探索
			function<BookRes()> to_sfen_func;
			to_sfen_func = [&]()
			{
				auto it = book.find(pos);
				// この局面が定跡登録されてなければ戻る。
				if (it == book.end())
					return BOOKRES_UNFILLED;
				auto& be = *it;
				// 探索済み局面(BookEntryのplyをフラグ代わり)なら戻る
				if (be.ply == 0)
					return BOOKRES_DUPEPOS;
				// move_listが空なら戻る
				if (be.move_list.size() == 0)
					return BOOKRES_EMPTYLIST;
				// 到達済み局面のフラグ立て
				be.ply = 0;
				// 最善手が駄目なら戻る
				auto& bp0 = be.move_list[0];
				int evaldiff = ~pos.side_to_move() ? evalblackdiff : evalwhitediff;
				int evallimit = ~pos.side_to_move() ? evalblacklimit : evalwhitelimit;
				if (bp0.depth < depthlimit)
					return BOOKRES_DEPTHLIMIT;
				if (bp0.value < evallimit)
					return BOOKRES_EVALLIMIT;
				// 最善手以外に何番目の候補手まで探索するか
				size_t imax = 1;
				for (;
					imax < be.move_list.size() &&
					be.move_list[imax].value >= evallimit &&
					be.move_list[imax].value + evaldiff >= bp0.value;
					++imax);
				// 先の局面を探索
				int ply = pos.game_ply();
				for (size_t i = 0; i < imax; ++i)
				{
					auto& bp = be.move_list[i];
					Move nowMove = bp.bestMove;
					// 探索スタック積み
					sf.push_back(to_movestr(pos, nowMove, m.empty() ? MOVE_NONE : m.back()));
					m.push_back(nowMove);
					pos.do_move(nowMove, si[ply]);
					// 先の局面で駄目出しされたら、その局面までの手順を出力
					auto res = (ply >= moves) ? BOOKRES_MOVELIMIT : to_sfen_func();
					if (res != BOOKRES_SUCCESS)
						printstack(fs, res);
					// 探索スタック崩し
					pos.undo_move(nowMove);
					m.pop_back();
					sf.pop_back();
				}
				return BOOKRES_SUCCESS;
			};

			// 探索開始
			istringstream ssop(opening);
			string opsfen;
			getline(ssop, opsfen, ',');
			do
			{
				sf.clear();
				m.clear();
				istringstream ssopsfen(opsfen);
				inisfen = fetch_initialpos(pos, ssopsfen);
				string movetoken;
				while (ssopsfen >> movetoken)
				{
					Move _mv = pos.move16_to_move(move_from_usi(movetoken));
					if (!(is_ok(_mv) && pos.pseudo_legal(_mv) && pos.legal(_mv)))
						break;
					sf.push_back(to_movestr(pos, _mv, m.empty() ? MOVE_NONE : m.back()));
					m.push_back(_mv);
					pos.do_move(_mv, si[pos.game_ply()]);
				}
				BookRes res = to_sfen_func();
				if (res != BOOKRES_SUCCESS)
					printstack(fs, res);
			} while (getline(ssop, opsfen, ','));

			// 探索終了
			fs.close();
			cout << ".finished!" << endl;
			cout << count_sfens << " lines exported." << endl;

		} else if (book_merge)
		{
			// 定跡のマージ
			MemoryBook book[3];
			string book_name[3];
			is >> book_name[0] >> book_name[1] >> book_name[2];
			if (book_name[2] == "")
			{
				cout << "Error! book name is empty." << endl;
				return;
			}
			cout << "book merge from " << book_name[0] << " and " << book_name[1] << " to " << book_name[2] << endl;
			for (int i = 0; i < 2; ++i)
			{
				cout << "reading book" << i << ".";
				if (read_book(book_name[i], book[i]) != 0)
					return;
				cout << ".done." << endl;
			}

			// 読み込めたので合体させる。
			cout << "merge..";

			// 同一nodeと非同一nodeの統計用
			// diffrent_nodes0 = book0側にのみあったnodeの数
			// diffrent_nodes1 = book1側にのみあったnodeの数
			u64 same_nodes = 0;
			u64 diffrent_nodes0 = 0, diffrent_nodes1 = 0;
			// 領域圧縮頻度しきい値、時々入力側の占有領域を整理してあげる。
			const u64 merge_shrink_threshold = 65536;
			u64 b0_shrink_count = 0, b1_shrink_count = 0;

			// マージ
			while (!book[0].book_body.empty() && !book[1].book_body.empty())
			{
				auto& b0front = book[0].book_body.front();
				auto& b1front = book[1].book_body.front();
				if (b0front == b1front)
				{
					// 同じ局面があったので、良いほうをbook2に突っ込む。
					same_nodes++;
					BookEntry be = move(b0front);
					be.update(move(b1front));
					book[2].add(be);
					book[0].book_body.pop_front();
					if (++b0_shrink_count >= merge_shrink_threshold)
					{
						book[0].book_body.shrink_to_fit();
						b0_shrink_count = 0;
					}
					book[1].book_body.pop_front();
					if (++b1_shrink_count >= merge_shrink_threshold)
					{
						book[1].book_body.shrink_to_fit();
						b1_shrink_count = 0;
					}
				} else if (b0front < b1front)
				{
					// book0からbook2に突っ込む
					diffrent_nodes0++;
					book[2].add(move(b0front));
					book[0].book_body.pop_front();
					if (++b0_shrink_count >= merge_shrink_threshold)
					{
						book[0].book_body.shrink_to_fit();
						b0_shrink_count = 0;
					}
				} else
				{
					// book1からbook2に突っ込む
					diffrent_nodes1++;
					book[2].add(move(b1front));
					book[1].book_body.pop_front();
					if (++b1_shrink_count >= merge_shrink_threshold)
					{
						book[1].book_body.shrink_to_fit();
						b1_shrink_count = 0;
					}
				}
			}
			// book0側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[0].book_body.empty())
			{
				diffrent_nodes0++;
				book[2].add(book[0].book_body.front());
				book[0].book_body.pop_front();
				if (++b0_shrink_count >= merge_shrink_threshold)
				{
					book[0].book_body.shrink_to_fit();
					b0_shrink_count = 0;
				}
			}
			// book1側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[1].book_body.empty())
			{
				diffrent_nodes1++;
				book[2].add(book[1].book_body.front());
				book[1].book_body.pop_front();
				if (++b1_shrink_count >= merge_shrink_threshold)
				{
					book[1].book_body.shrink_to_fit();
					b1_shrink_count = 0;
				}
			}
			cout << "..done" << endl;

			cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << endl;

			cout << "write..";
			write_book(book_name[2], book[2]);
			cout << "..done!" << endl;

		} else if (book_sort)
		{
			// 定跡のsort, normalization(n11n)
			MemoryBook book;
			string book_src, book_dst;
			is >> book_src >> book_dst;
			is_ready();
			cout << "book sort from " << book_src << " , write to " << book_dst << endl;
			Book::read_book(book_src, book, false, true);

			cout << "write..";
			write_book(book_dst, book);
			cout << "..done!" << endl;

		} else
		{
			cout << "usage" << endl;
			cout << "> makebook from_sfen book.sfen book.db moves 24" << endl;
			cout << "> makebook think book.sfen book.db moves 16 depth 18" << endl;
			cout << "> makebook merge book_src1.db book_src2.db book_merged.db" << endl;
			cout << "> makebook sort book_src.db book_sorted.db" << endl;
			cout << "> makebook to_sfen book.db book.sfen moves 256 evaldiff 30 evalblacklimit 0 evalwhitelimit -140 depthlimit 16" << endl;
		}
	}
#endif

	// 定跡ファイルの読み込み(book.db)など。
	int read_book(const std::string& filename, MemoryBook& book, bool on_the_fly, bool sfen_n11n)
	{
		// 読み込み済であるかの判定
		if (book.book_name == filename)
			return 0;

		// 別のファイルを開こうとしているなら前回メモリに丸読みした定跡をクリアしておかないといけない。
		if (book.book_name != "")
			book.clear();

		// ファイルだけオープンして読み込んだことにする。
		if (on_the_fly)
		{
			if (book.fs.is_open())
				book.fs.close();

			book.fs.open(filename, ios::in);
			if (book.fs.fail())
			{
				cout << "info string Error! : can't read " + filename << endl;
				return 1;
			}

			book.on_the_fly = true;
			book.book_name = filename;
			return 0;
		}

		// OnTheFly を行わない場合、オーバーヘッド（2倍～数倍？）を極力抑えるために低レベルな処理を織り交ぜる

		// ファイルハンドラ
		FILE * _fp;
		
		// 読み込みバッファ
		const size_t _buffersize = 16 * 1024 * 1024; // 16MiB
		char * _buffer = new char[_buffersize];
		// 読み込み範囲始点、読み込み範囲終点、バッファ内最終行始点、バッファ終点
		size_t _p0 = 0, _p1 = 0, _lastheadp = 0, _endp = 0;
		// バッファサイズ超過行フラグ
		bool _nolf = false;

		// バッファへの次ブロック読み込み
		auto _readblock = [&]()
		{
			size_t _tmpp = 0, _remain = 0;
			char * __p;
			// バッファサイズを超過する行の先頭ブロックを読んだ後に呼ばれた場合は、行末まで読み捨てる
			while(_nolf)
			{
				_endp = fread(_buffer, sizeof(char), _buffersize - 1, _fp);
				_buffer[_endp] = NULL;
				// そのままファイル終端に達したら終了
				if (_endp == 0)
					return;
				// 行末が無いか探す、それでも無ければ次ブロックの読み込み
				__p = _buffer;
				for(_p0 = 0; _p0 < _endp; ++_p0)
					if (*__p++ == '\n')
					{
						_nolf = false;
						break;
					}
			}
			// 読み残し領域を先頭にコピー
			if (_p0 < _endp)
				memcpy(_buffer, _buffer + _p0, _tmpp = _endp - _p0);
			_p0 = _p1 = 0;
			// バッファの残りサイズ
			_remain = _buffersize - _tmpp;
			// 1文字分の余裕を残して読み込み、空いた1文字はNULで埋める
			_endp = _tmpp + (_remain > 1 ? fread(_buffer + _tmpp, sizeof(char), _remain - 1, _fp) : 0);
			__p = _buffer + _endp;
			*__p = NULL;
			// 最終行の行頭位置検出
			for (_lastheadp = _endp; _lastheadp > 0 && *--__p != '\n'; --_lastheadp);
			// 全く改行文字が存在しなければ、次回は行末まで読み捨てる処理を行うのでフラグを立てる
			if (_lastheadp == 0 && *_buffer != '\n')
				_nolf = true;
		};

		// バッファ内の次行範囲探索、必要に応じてバッファへの次ブロック読み込み
		auto _nextrange = [&]()
		{
			char * _p;
			// 前回の終端を始点として調べ始める
			_p0 = _p1;
			// 次の行頭位置を調べる
			_p = _buffer + _p0;
			while (_p0 < _lastheadp && *_p++ == '\n') ++_p0;
			// バッファを読み終わるなら次ブロックをバッファに読み込み
			if (_p0 >= _lastheadp)
				_readblock();
			// 行末位置を調べる
			_p1 = _p0;
			_p = _buffer + _p1;
			while (_p1 < _lastheadp && *_p++ != '\n') ++_p1;
		};

		_fp = fopen(filename.c_str(), "r");

		if (_fp == NULL)
		{
			cout << "info string Error! : can't read " + filename << endl;
			return 1;
		}

		// 先頭行読み込み
		_nextrange();
		while (_p1 > 0)
		{
			// "sfen " で始まる25バイト以上の行を見つける
			if (memcmp(_buffer + _p0, "sfen ", 5) || _p0 + 24 > _p1)
			{
				_nextrange();
				continue;
			}
			// BookEntry 構築
			BookEntry be(_buffer + _p0 + 5, _p1 - _p0 - 5, sfen_n11n);
			// BookEntry への BookPos要素充填
			while (true)
			{
				_nextrange();
				// ファイル終端に達したら終了
				if (_p1 == 0)
					break;
				// 次の sfen が始まったら終了
				if (!memcmp(_buffer + _p0, "sfen ", 5))
					break;
				// 行頭文字がおかしければスキップして次の行を調べる
				char c = _buffer[_p0];
				if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
					continue;
				// BookPos 構築
				BookPos bp(_buffer + _p0);
				// BookEntry に追加
				be.move_list.push_back(bp);
			}
			// おかしければ登録せずに次へ
			if (be.sfenPos.empty() || be.move_list.empty())
				continue;
			// 採択確率を計算して、かつ、採択回数でsortしておく
			// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
			be.calc_prob();
			// MemoryBook に登録
			book.add(be);
		}

		fclose(_fp);

		delete[] _buffer;

		// 全体のソート、重複局面の除去
		book.intl_uniq();

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book.book_name = filename;

		return 0;
	}

	// 定跡ファイルの書き出し
	int write_book(const std::string& filename, MemoryBook& book)
	{
		// 全体のソートと重複局面の除去
		book.intl_uniq();

		// オーバーヘッド（2倍～数倍？）を防ぐため、低レベルな処理も織り交ぜる

		// 1要素あたりの最大サイズ
		const size_t _entrymaxsize = 65536; // 64kiB
		// 領域確保係数
		const size_t _bufmulti = 256;
		// 最大バッファ領域サイズ
		const size_t _bufmaxsize = _entrymaxsize * _bufmulti; // 16MiB
		// バッファ領域サイズ
		size_t _bufsize = _entrymaxsize * min(_bufmulti, book.book_body.size() + 1);
		// バッファ払い出し閾値
		size_t _bufth = _bufsize - _entrymaxsize;
		// バッファ確保
		char * _buf = new char[_bufsize];

		// 文字列バッファ char[] に BookEntry を書き出し
		// BookEntry 毎に書き出しを行う
		size_t _p;
		auto _render_be = [&](const BookEntry& be)
		{
			// Move 書き出し
			auto _render_move = [&](const Move m)
			{
				if (!is_ok(m))
				{
					// 頻度が低いのでstringで手抜き
					string str;
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
					}
					char_traits<char>::copy(_buf + _p, str.c_str(), str.size());
					_p += str.size();
				}
				else if (is_drop(m))
				{
					Square sq_to = move_to(m);
					_buf[_p++] = USI_PIECE[(m >> 6) & 30]; // == USI_PIECE[(move_dropped_piece(m) & 15) * 2];
					_buf[_p++] = '*';
					_buf[_p++] = (char)('1' + file_of(sq_to));
					_buf[_p++] = (char)('a' + rank_of(sq_to));
				}
				else
				{
					Square sq_from = move_from(m), sq_to = move_to(m);
					_buf[_p++] = (char)('1' + file_of(sq_from));
					_buf[_p++] = (char)('a' + rank_of(sq_from));
					_buf[_p++] = (char)('1' + file_of(sq_to));
					_buf[_p++] = (char)('a' + rank_of(sq_to));
					if (is_promote(m))
						_buf[_p++] = '+';
				}
			};
			// 整数値書き出し(sprintf_s を使うとこのコードより2倍～数倍？重い)
			auto _render_u16_1 = [&](u16 i)
			{
				if(i > 9u)
					i %= 10u;
				_buf[_p++] = '0' + i;
			};
			auto _render_u16_2l = [&](u16 i)
			{
				_render_u16_1(i / 10u);
				_render_u16_1(i);
			};
			auto _render_u16_2u = [&](u16 i)
			{
				if(i > 9u)
					_render_u16_1(i / 10u);
				_render_u16_1(i);
			};
			auto _render_u16_4l = [&](u16 i)
			{
				_render_u16_2l(i / 100u);
				_render_u16_2l(i);
			};
			auto _render_u16_4u = [&](u16 i)
			{
				if(i > 99u)
				{
					_render_u16_2u(i / 100u);
					_render_u16_2l(i);
				}
				else
					_render_u16_2u(i);
			};
			auto _render_u32_8l = [&](u32 i)
			{
				if (i > 99999999ul)
					i %= 100000000ul;
				_render_u16_4l((u16)(i / 10000ul));
				_render_u16_4l((u16)(i % 10000ul));
			};
			auto _render_u32_8u = [&](u32 i)
			{
				if(i > 9999ul)
				{
					_render_u16_4u((u16)(i / 10000ul));
					_render_u16_4l((u16)(i % 10000ul));
				}
				else
					_render_u16_4u((u16)i);
			};
			auto _render_u64_16l = [&](u64 i)
			{
				if(i > 9999999999999999ull)
					i %= 10000000000000000ull;
				_render_u32_8l((u32)(i / 100000000ull));
				_render_u32_8l((u32)(i % 100000000ull));
			};
			auto _render_u64_16u = [&](u64 i)
			{
				if(i > 99999999ull)
				{
					_render_u32_8u((u32)(i / 100000000ull));
					_render_u32_8l((u32)(i % 100000000ull));
				}
				else
					_render_u32_8u((u32)i);
			};
			auto _render_u32 = [&](u32 i)
			{
				if (i > 99999999ul)
				{
					_render_u32_8u(i / 100000000ul);
					_render_u32_8l(i % 100000000ul);
				}
				else
					_render_u32_8u(i);
			};
			auto _render_s32 = [&](s32 i)
			{
				if (i < 0)
					_buf[_p++] = '-';
				_render_u32((u32)(i < 0 ? -i : i));
			};
			auto _render_u64 = [&](u64 i)
			{
				if(i > 9999999999999999ull)
				{
					_render_u64_16u(i / 10000000000000000ull);
					_render_u64_16l(i % 10000000000000000ull);
				}
				else
					_render_u64_16u(i);
			};
			auto _render_s64 = [&](s64 i)
			{
				if (i < 0)
					_buf[_p++] = '-';
				_render_u64((u64)(i < 0 ? -i : i));
			};

			// 先頭の"sfen "と末尾の指し手手数を除いた sfen文字列 の長さは最大で 95bytes?
			// "+l+n+sgkg+s+n+l/1+r5+b1/+p+p+p+p+p+p+p+p+p/9/9/9/+P+P+P+P+P+P+P+P+P/1+B5+R1/+L+N+SGKG+S+N+L b -"
			// 128bytes を超えたら明らかにおかしいので抑止。
			_buf[_p++] = 's';
			_buf[_p++] = 'f';
			_buf[_p++] = 'e';
			_buf[_p++] = 'n';
			_buf[_p++] = ' ';
			char_traits<char>::copy(_buf + _p, be.sfenPos.c_str(), min(be.sfenPos.size(), (size_t)128));
			_p += min(be.sfenPos.size(), (size_t)128);
			_buf[_p++] = ' ';
			_render_s32(be.ply);
			_buf[_p++] = '\n';
			// BookPosは改行文字を含めても 62bytes を超えないので、
			// be.move_list.size() <= 1000 なら64kiBの _buffer を食い尽くすことは無いはず
			// 1局面の合法手の最大は593（歩不成・2段香不成・飛不成・角不成も含んだ場合、以下局面例）なので、
			// sfen 8R/kSS1S1K2/4B4/9/9/9/9/9/3L1L1L1 b RB4GS4NL18P 1
			// be.move_list.size() > 1000 なら安全のため出力しない
			if (be.move_list.size() <= 1000)
				for (auto& bp : be.move_list)
				{
					_render_move(bp.bestMove);
					_buf[_p++] = ' ';
					_render_move(bp.nextMove);
					_buf[_p++] = ' ';
					_render_s32(bp.value);
					_buf[_p++] = ' ';
					_render_s32(bp.depth);
					_buf[_p++] = ' ';
					_render_u64(bp.num);
					_buf[_p++] = '\n';
				}
			_buf[_p] = NULL; // 念のためNULL文字出力
		};

		FILE * fp;
		if ((fp = fopen(filename.c_str(), "w")) == NULL)
		{
			cout << "info string Error! : can't write " + filename << endl;
			return 1;
		}

		// バージョン識別用文字列
		const char _headverstr[] = "#YANEURAOU-DB2016 1.00\n";
		fwrite(_headverstr, sizeof(char), sizeof(_headverstr) - 1, fp);

		_buf[_p = 0] = NULL;
		for (BookEntry& be : book.book_body)
		{
			// 指し手のない空っぽのentryは書き出さないように。
			if (be.move_list.empty()) continue;
			// 採択回数でソートしておく。
			be.sort_pos();
			// 出力
			_render_be(be);
			// _bufth (16320kiB) 以上まで出力が溜まったらファイルに書き出し
			if (_p > _bufth)
			{
				fwrite(_buf, sizeof(char), _p, fp);
				_buf[_p = 0] = NULL;
			}
		}
		// 残りの出力をファイルに書き出し
		if (_p > 0)
			fwrite(_buf, sizeof(char), _p, fp);

		fclose(fp);

		// バッファ開放
		delete[] _buf;

		return 0;
	}

	// char文字列 の BookPos 化
	void BookEntry::set(const char* sfen, const size_t length, const bool sfen_n11n)
	{
		if(sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。		
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)		
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(string(sfen, length));
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		}
		else
		{
			auto cur = trimlen_sfen(sfen, length);
			sfenPos = string(sfen, cur);
			ply = strtol(sfen + cur, NULL, 10);
		}
	}

	// ストリームからの BookEntry 読み込み
	void BookEntry::set(std::istream& is, char* _buffer, const size_t _buffersize, const bool sfen_n11n)
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
			if (!std::getline(is, line))
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
	void BookEntry::incpos(std::istream& is, char* _buffer, const size_t _buffersize)
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
			BookPos bp(_buffer);
			move_list.push_back(bp);
		}
	}

	// バイト文字列からの BookPos 読み込み
	void BookPos::set(char* p)
	{
		// 指し手解析部
		auto _mvparse = [](char* p) -> Move
		{
			char c0 = *p++;
			if (c0 >= '1' && c0 <= '9')
			{
				char c1 = *p++; if (c1 < 'a' || c1 > 'i') return MOVE_NONE;
				char c2 = *p++; if (c2 < '1' || c2 > '9') return MOVE_NONE;
				char c3 = *p++; if (c3 < 'a' || c3 > 'i') return MOVE_NONE;
				char c4 = *p++;
				if (c4 == '+')
					return make_move_promote(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
				else
					return make_move(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
			else if (c0 >= 'A' && c0 <= 'Z')
			{
				char c1 = *p++; if (c1 != '*') return MOVE_NONE;
				char c2 = *p++; if (c2 < '1' || c2 > '9') return MOVE_NONE;
				char c3 = *p++; if (c3 < 'a' || c3 > 'i') return MOVE_NONE;
				for (int i = 1; i <= 7; ++i)
					if (PieceToCharBW[i] == c0)
						return make_move_drop((Piece)i, toFile(c2) | toRank(c3));
			}
			else if (c0 == '0' || c0 >= 'a' && c0 <= 'z')
			{
				if (!memcmp(p, "win", 3)) return MOVE_WIN;
				if (!memcmp(p, "null", 4)) return MOVE_NULL;
				if (!memcmp(p, "pass", 4)) return MOVE_NULL;
				if (!memcmp(p, "0000", 4)) return MOVE_NULL;
			}
			return MOVE_NONE;
		};

		// 解析実行
		// 0x00 - 0x1f, 0x21 - 0x29, 0x80 - 0xff の文字が現れ次第中止
		// 特に、NULL, CR, LF 文字に反応する事を企図。TAB 文字でも中止。SP 文字連続でも中止。
		if (*p < '*') return;
		bestMove = _mvparse(p);
		while (true) if (*++p < '*') { if (*p != ' ' || *++p < '*') return; break; }
		nextMove = _mvparse(p);
		while (true) if (*++p < '*') { if (*p != ' ' || *++p < '*') return; break; }
		value = strtol(p, NULL, 10);
		while (true) if (*++p < '*') { if (*p != ' ' || *++p < '*') return; break; }
		depth = strtol(p, NULL, 10);
		while (true) if (*++p < '*') { if (*p != ' ' || *++p < '*') return; break; }
		num = strtoull(p, NULL, 10);
	}

	MemoryBook::BookType::iterator MemoryBook::find(const Position& pos)
	{
		// 定跡がないならこのまま返る。(sfen()を呼び出すコストの節約)
		if (!on_the_fly && book_body.size() == 0)
			return book_body.end();

		BookType::iterator it;

		if (on_the_fly)
		{
			// ディスクから読み込むなら、いずれにせよ、book_bodyをクリアして、
			// ディスクから読み込んだエントリーをinsertしてそのiteratorを返すべき。
			clear();

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
				string line;

				fs.seekg(max(s64(0), (s64)seek_from - 80), fstream::beg);
				getline(fs, line); // 1行読み捨てる

				// getlineはeof()を正しく反映させないのでgetline()の返し値を用いる必要がある。
				while (getline(fs, line))
				{
					if (!line.compare(0, 5, "sfen "))
						return trim_sfen(line.substr(5));
					// "sfen "という文字列は取り除いたものを返す。
					// 手数の表記も取り除いて比較したほうがいい。
					// ios::binaryつけているので末尾に'\R'が付与されている。禿げそう。
				}
				return string();
			};

			// バイナリサーチ

			u64 s = 0, e = file_size, m;

			while (true)
			{
				m = (s + e) / 2;

				auto sfen2 = next_sfen(m);
				if (sfen2 == "" || sfen < sfen2)
				{ // 左(それより小さいところ)を探す
					e = m;
				} else if (sfen > sfen2)
				{ // 右(それより大きいところ)を探す
					s = u64(fs.tellg() - file_start);
				} else
				{
					// 見つかった！
					break;
				}

				// 40バイトより小さなsfenはありえないので探索範囲がこれより小さいなら終了。
				// ただしs = 0のままだと先頭が探索されていないので..
				// s,eは無符号型であることに注意。if (s-40 < e) と書くとs-40がマイナスになりかねない。
				if (s + 40 > e)
				{
					if (s != 0 || e != 0)
					{
						// 見つからなかった
						return book_body.end();
					}

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
			be.calc_prob();
			add(be);

			it = book_body.begin();

		} else
		{
			// on the flyではない場合
			it = intl_find(pos);
		}


		if (it != book_body.end())
		{
			// 定跡のMoveは16bitであり、rootMovesは32bitのMoveであるからこのタイミングで補正する。
			for (auto& m : it->move_list)
				m.bestMove = pos.move16_to_move(m.bestMove);
		}
		return it;
	}

	// 出力ストリーム
	// write_book では最適化のため現在使われていない
	ostream& operator << (ostream& os, const BookPos& bp)
	{
		os << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << ' ' << bp.depth << ' ' << bp.num << "\n";
		return os;
	}
	ostream& operator << (ostream& os, const BookEntry& be)
	{
		os << "sfen " << be.sfenPos << " " << be.ply << "\n";
		for (auto& bp : be.move_list)
			os << bp;
		return os;
	}

	// sfen文字列のゴミを除いた長さ
	const std::size_t trimlen_sfen(const char* sfen, const size_t length)
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
	const std::size_t trimlen_sfen(const std::string sfen)
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
	const std::pair<const std::string, const int> split_sfen(const std::string sfen)
	{
		auto cur = trimlen_sfen(sfen);
		std::string s = sfen;
		s.resize(cur);
		int ply = strtol(sfen.c_str() + cur, NULL, 10);
		return std::make_pair(s, ply);
	}

}

