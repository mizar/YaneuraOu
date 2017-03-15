#include "../shogi.h"

#include <fstream>
#include <sstream>
#include <unordered_set>
#include <iomanip>

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
				{
					cout << "..but , create new file." << endl;
				} else
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

				size_t compos;

				// "#"以降読み捨て
				if ((compos = sfen.find("#")) < sfen.length()) { sfen.resize(compos); }

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
			if (from_thinking && start_moves == 1 && thinking_sfens.empty()) {
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
					} else {
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
			if (from_sfen) {
				cout << "write..";
				write_book(book_name, book);
				cout << "finished." << endl;
			}

		} else if (to_sfen || to_kif1 || to_kif2 || to_csa1) {
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
					for (auto& s : sf) { os << " " << s; }
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
			to_sfen_func = [&]() {
				auto it = book.find(pos);
				// この局面が定跡登録されてなければ戻る。
				if (it == book.end()) { return BOOKRES_UNFILLED; }
				auto& be = *it;
				// 探索済み局面(BookEntryのplyをフラグ代わり)なら戻る
				if (be.ply == 0) { return BOOKRES_DUPEPOS; }
				// move_listが空なら戻る
				if (be.move_list.size() == 0) { return BOOKRES_EMPTYLIST; }
				// 到達済み局面のフラグ立て
				be.ply = 0;
				// 最善手が駄目なら戻る
				auto& bp0 = be.move_list[0];
				int evaldiff = ~pos.side_to_move() ? evalblackdiff : evalwhitediff;
				int evallimit = ~pos.side_to_move() ? evalblacklimit : evalwhitelimit;
				if (bp0.depth < depthlimit) { return BOOKRES_DEPTHLIMIT; }
				if (bp0.value < evallimit) { return BOOKRES_EVALLIMIT; }
				// 最善手以外に何番目の候補手まで探索するか
				size_t imax = 1;
				for (;
					imax < be.move_list.size() &&
					be.move_list[imax].value >= evallimit &&
					be.move_list[imax].value + evaldiff >= bp0.value;
					++imax) {}
				// 先の局面を探索
				int ply = pos.game_ply();
				for (size_t i = 0; i < imax; ++i) {
					auto& bp = be.move_list[i];
					Move nowMove = bp.bestMove;
					// 探索スタック積み
					sf.push_back(to_movestr(pos, nowMove, m.empty() ? MOVE_NONE : m.back()));
					m.push_back(nowMove);
					pos.do_move(nowMove, si[ply]);
					// 先の局面で駄目出しされたら、その局面までの手順を出力
					auto res = (ply >= moves) ? BOOKRES_MOVELIMIT : to_sfen_func();
					if (res != BOOKRES_SUCCESS) {
						printstack(fs, res);
					}
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
				if (res != BOOKRES_SUCCESS) {
					printstack(fs, res);
				}
			} while (getline(ssop, opsfen, ','));

			// 探索終了
			fs.close();
			cout << ".finished!" << endl;
			cout << count_sfens << " lines exported." << endl;

		} else if (book_merge) {
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
			while (!book[0].book_body.empty() && !book[1].book_body.empty()) {
				auto& b0front = book[0].book_body.front();
				auto& b1front = book[1].book_body.front();
				if (b0front == b1front) {
					// 同じ局面があったので、良いほうをbook2に突っ込む。
					same_nodes++;
					BookEntry be = b0front;
					be.update(b1front);
					book[2].add(be);
					book[0].book_body.pop_front();
					if (++b0_shrink_count >= merge_shrink_threshold) { book[0].book_body.shrink_to_fit(); b0_shrink_count = 0; }
					book[1].book_body.pop_front();
					if (++b1_shrink_count >= merge_shrink_threshold) { book[1].book_body.shrink_to_fit(); b1_shrink_count = 0; }
				} else if (b0front < b1front) {
					// book0からbook2に突っ込む
					diffrent_nodes0++;
					book[2].add(b0front);
					book[0].book_body.pop_front();
					if (++b0_shrink_count >= merge_shrink_threshold) { book[0].book_body.shrink_to_fit(); b0_shrink_count = 0; }
				} else {
					// book1からbook2に突っ込む
					diffrent_nodes1++;
					book[2].add(b1front);
					book[1].book_body.pop_front();
					if (++b1_shrink_count >= merge_shrink_threshold) { book[1].book_body.shrink_to_fit(); b1_shrink_count = 0; }
				}
			}
			// book0側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[0].book_body.empty()) {
				diffrent_nodes0++;
				book[2].add(book[0].book_body.front());
				book[0].book_body.pop_front();
				if (++b0_shrink_count >= merge_shrink_threshold) { book[0].book_body.shrink_to_fit(); b0_shrink_count = 0; }
			}
			// book1側でまだ突っ込んでいないnodeを、book2に突っ込む
			while (!book[1].book_body.empty()) {
				diffrent_nodes1++;
				book[2].add(book[1].book_body.front());
				book[1].book_body.pop_front();
				if (++b1_shrink_count >= merge_shrink_threshold) { book[1].book_body.shrink_to_fit(); b1_shrink_count = 0; }
			}
			cout << "..done" << endl;

			cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << endl;

			cout << "write..";
			write_book(book_name[2], book[2]);
			cout << "..done!" << endl;

		} else if (book_sort) {
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

		} else {
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

		std::deque<string> lines;
		if (read_all_lines(filename, lines))
		{
			cout << "info string Error! : can't read " + filename << endl;
			//      exit(EXIT_FAILURE);
			return 1; // 読み込み失敗
		}

		string sfen;
		BookEntry& be = BookEntry();
		Position pos;

		// 入力バッファの領域を縮小させる頻度のしきい値
		const u64 read_shrink_threshold = 1048576;
		u64 shrink_count = 0;

		while (!lines.empty())
		{
			auto line = std::move(lines.front());
			lines.pop_front();

			// 入力バッファの縮小
			if (++shrink_count >= read_shrink_threshold)
			{
				lines.shrink_to_fit();
				shrink_count = 0;
			}

			// バージョン識別文字列(とりあえず読み飛ばす)
			if (line.length() >= 1 && line[0] == '#')
				continue;

			// コメント行(とりあえず読み飛ばす)
			if (line.length() >= 2 && line.substr(0, 2) == "//")
				continue;

			// "sfen "で始まる行は局面のデータであり、sfen文字列が格納されている。
			if (line.length() >= 5 && line.substr(0, 5) == "sfen ")
			{
				// ひとつ前のsfen文字列に対応するものが終わったということなので採択確率を計算して、かつ、採択回数でsortしておく
				// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
				be.calc_prob();
				if (!be.move_list.empty()) { book.add(be); }

				sfen = line.substr(5, line.length() - 5); // 新しいsfen文字列を"sfen "を除去して格納

				// sfen文字列は手駒の表記に揺れがある。
				// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)
				// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
				if (sfen_n11n) { pos.set(sfen); sfen = pos.sfen(); }

				BookEntry be_(split_sfen(sfen));
				be = be_;

				continue;
			}

			BookPos bp(line);
			be.insert_book_pos(bp);
		}

		// ファイルが終わるときにも最後の局面に対するcalc_prob,book.addが必要。
		be.calc_prob();
		if (!be.move_list.empty()) { book.add(be); }

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book.book_name = filename;

		// 全体のソート、重複局面の除去
		book.intl_uniq();

		return 0;
	}

	// 定跡ファイルの書き出し
	int write_book(const std::string& filename, MemoryBook& book)
	{
		fstream fs;
		fs.open(filename, ios::out);

		// バージョン識別用文字列
		fs << "#YANEURAOU-DB2016 1.00" << endl;

		// 全体のソートと重複局面の除去
		book.intl_uniq();

		for (BookEntry be : book.book_body)
		{
			// 指し手のない空っぽのentryは書き出さないように。
			if (be.move_list.size() == 0) continue;
			// 採択回数でソートしておく。
			be.sort_pos();

			fs << be;
		}

		fs.close();

		return 0;
	}

	MemoryBook::BookType::iterator MemoryBook::find(const Position& pos)
	{
		// 定跡がないならこのまま返る。(sfen()を呼び出すコストの節約)
		if (!on_the_fly && book_body.size() == 0)
			return book_body.end();

		auto sfen = pos.sfen();
		BookType::iterator it;

		if (on_the_fly)
		{
			// ディスクから読み込むなら、いずれにせよ、book_bodyをクリアして、
			// ディスクから読み込んだエントリーをinsertしてそのiteratorを返すべき。
			clear();

			// 末尾の手数は取り除いておく。
			// read_book()で取り除くと、そのあと書き出すときに手数が消失するのでまずい。(気がする)
			sfen = trim_sfen(sfen);

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
					if (!line.compare(0, 4, "sfen"))
						return trim_sfen(line.substr(5));
					// "sfen"という文字列は取り除いたものを返す。
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
				} else {
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

			// read_bookとほとんど同じ読み込み処理がここに必要。辛い。

			BookEntry be(split_sfen(sfen));

			while (!fs.eof())
			{
				string line;
				getline(fs, line);

				// バージョン識別文字列(とりあえず読み飛ばす)
				if (line.length() >= 1 && line[0] == '#')
					continue;

				// コメント行(とりあえず読み飛ばす)
				if (line.length() >= 2 && line.substr(0, 2) == "//")
					continue;

				// 次のsfenに遭遇したらこれにて終了。
				if (line.length() >= 5 && line.substr(0, 5) == "sfen ")
				{
					break;
				}

				BookPos bp(line);

				be.insert_book_pos(bp);

			}
			// ファイルが終わるときにも最後の局面に対するcalc_probが必要。
			be.calc_prob();

			add(be);

			it = book_body.begin();

		} else {

			// on the flyではない場合
			it = intl_find(sfen);
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
	std::ostream& operator << (std::ostream& os, const BookPos& bp) {
		os << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << ' ' << bp.depth << ' ' << bp.num << std::endl;
		return os;
	}
	std::ostream& operator << (std::ostream& os, const BookEntry& be) {
		os << "sfen " << be.sfenPos << " " << be.ply << std::endl;
		for (auto& bp : be.move_list) {	os << bp; }
		return os;
	}

	// sfen文字列のゴミを除いた長さ
	const std::size_t trimlen_sfen(const std::string sfen)
	{
		auto cur = sfen.length();
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
		std::istringstream ss(sfen);
		int ply;
		ss.seekg(cur);
		ss >> ply;
		return std::make_pair(s, ply);
	}

}

