#include <fstream>
#include <sstream>
#include <unordered_set>
#include <iomanip>
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

using namespace std;
using std::cout;

void is_ready();

namespace Book
{

#ifdef ENABLE_MAKEBOOK_CMD
	// ----------------------------------
	// USI拡張コマンド "bookutil"(定跡ツール)
	// ----------------------------------
	// 局面を与えて、その局面で思考させるために、やねうら王2016Mid以降が必要。
#if defined(EVAL_LEARN) && (defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE) || defined(YANEURAOU_2017_EARLY_ENGINE) || defined(YANEURAOU_2017_GOKU_ENGINE))
	struct dMultiThinkBook: public MultiThink
	{
		dMultiThinkBook(int search_depth_, dMemoryBook & book_)
			: search_depth(search_depth_), book(book_), appended(false) {}

		virtual void thread_worker(size_t thread_id);

		// 定跡を作るために思考する局面
		vector<string> sfens;

		// 定跡を作るときの通常探索の探索深さ
		int search_depth;

		// メモリ上の定跡ファイル(ここに追加していく)
		dMemoryBook & book;

		// 前回から新たな指し手が追加されたかどうかのフラグ。
		bool appended;

		// 進捗表示の出力形式。
		int progress_type;

	};

	//  thread_id    = 0..Threads.size()-1
	//  search_depth = 通常探索の探索深さ
	void dMultiThinkBook::thread_worker(size_t thread_id)
	{
		// g_loop_maxになるまで繰り返し
		u64 id;
		if (progress_type != 0)
			cout << ("info string progress " + to_string(get_loop_count()) + " / " + to_string(get_loop_max()) + "\n") << flush;
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

			ctr_dbp_t move_list;

			int multi_pv = min((int)Options["MultiPV"], (int)th->rootMoves.size());
			for (int i = 0; i < multi_pv; ++i)
			{
				// 出現頻度は、バージョンナンバーを100倍したものにしておく)
				Move nextMove = (th->rootMoves[i].pv.size() >= 1) ? th->rootMoves[i].pv[1] : MOVE_NONE;
				dBookPos bp(th->rootMoves[i].pv[0], nextMove, th->rootMoves[i].score
					, search_depth, int(atof(ENGINE_VERSION) * 100));

				// MultiPVで思考しているので、手番側から見て評価値の良い順に並んでいることは保証される。
				// (書き出しのときに並び替えなければ)
				move_list.push_back(bp);
			}

			{
				unique_lock<Mutex> lk(io_mutex);
				// 前のエントリーは上書きされる。
				dBookEntry be(split_sfen(sfen), move_list);
				book.add(be, true);

				// 新たなエントリーを追加したのでフラグを立てておく。
				appended = true;
			}

			// 1局面思考するごとに'.'をひとつ出力する。
			if (progress_type == 0)
				cout << ".";
			else
				cout << ("info string progress " + to_string(get_loop_count()) + " / " + to_string(get_loop_max()) + "\n") << flush;
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
	size_t trimlen_sfen(const string sfen)
	{
		return trimlen_sfen(sfen.c_str(), sfen.length());
	}

	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const string trim_sfen(const string sfen)
	{
		string s = sfen;
		s.resize(trimlen_sfen(sfen));
		return s;
	}

	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const string sfen)
	{
		auto cur = trimlen_sfen(sfen);
		string s = sfen;
		s.resize(cur);
		int ply = QConv::atos32(sfen.c_str() + cur);
		return make_pair(s, ply);
	}

	// 棋譜文字列ストリームから初期局面を抽出して設定
	string fetch_initialpos(Position & pos, istream & is)
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
		} else do
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
	void bookutil_cmd(Position & pos, istringstream & is)
	{
		string token;
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

#if !defined(EVAL_LEARN) || !(defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE) || defined(YANEURAOU_2017_EARLY_ENGINE) || defined(YANEURAOU_2017_GOKU_ENGINE))
		if (from_thinking)
		{
			cout << "Error!:define EVAL_LEARN and ( YANEURAOU_2017_EARLY_ENGINE or GOKU_ENGINE ) " << endl;
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

			dMemoryBook book;

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

			vector<string> sfens;
			read_all_lines(sfen_name, sfens);

			cout << "parse.." << flush;

			// 思考すべき局面のsfen
			unordered_set<string> thinking_sfens;

			// StateInfoのスタック
			Search::StateStackPtr ssp;

			// 各行の局面をparseして読み込む(このときに重複除去も行なう)
			for (size_t k = 0; k < sfens.size(); ++k)
			{
				auto sfen = move(sfens[k]);

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
						cout << "illegal move : line = " << (k + 1) << " , " << sfen << " , move = " << token << endl;
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
						dBookPos bp(m[i], m[i + 1], VALUE_ZERO, 32, 1);
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
				dMultiThinkBook multi_think(depth, book);

				// 進捗表示形式
				multi_think.progress_type = progress_type;

				auto & sfens_ = multi_think.sfens;
				for (auto & s : thinking_sfens)
				{

					// この局面のいま格納されているデータを比較して、この局面を再考すべきか判断する。
					auto it = book.find(trim_sfen(s));

					// dMemoryBookにエントリーが存在しないなら無条件で、この局面について思考して良い。
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
				cout << "thinking sfen = " << endl;
				for (auto & s : sfens_)
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
					unique_lock<Mutex> lk(multi_think.io_mutex);
					// 前回書き出し時からレコードが追加された？
					if (multi_think.appended)
					{
						write_book(book_name, book);
						cout << ("S" + to_string(multi_think.get_remain_loop_count()) + "\n") << flush;
						multi_think.appended = false;
					} else
						// 追加されていないときは小文字のsマークを表示して
						// ファイルへの書き出しは行わないように変更。
						cout << "s\n" << flush;

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
			dMemoryBook book;
			if (read_book(book_name, book) != 0)
				cout << "..but , create kifs." << endl;
			cout << "..done. " << book.book_body.size() << " nodes loaded." << endl;

			is_ready();

			// カウンタ
			const u64 count_sfens_threshold = 10000;
			u64 count_sfens = 0, count_sfens_part = 0;

			vector<Move> m;
			vector<string> sf;	// sfen指し手文字列格納用

			vector<StateInfo> si(max(moves + 2, 258));

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
			auto printstack = [&](ostream & os, BookRes res)
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
					}
				}
				os << "\n";
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
				auto & be = *it;
				// 探索済み局面(BookEntryのplyをフラグ代わり)なら戻る
				if (be.ply == 0)
					return BOOKRES_DUPEPOS;
				// move_listが空なら戻る
				if (be.move_list.empty())
					return BOOKRES_EMPTYLIST;
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
					pos.do_move(nowMove, si.at(ply));
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
					pos.do_move(_mv, si.at(pos.game_ply()));
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
			dMemoryBook book[3];
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

			// マージ
			while (!book[0].book_body.empty() && !book[1].book_body.empty())
			{
				dBookEntry & b0front = book[0].book_body.front();
				dBookEntry & b1front = book[1].book_body.front();
				int cmp = b0front.compare(b1front);
				if (cmp == 0)
				{
					// 同じ局面があったので、良いほうをbook2に突っ込む。
					same_nodes++;
					dBookEntry be = move(b0front);
					be.select(move(b1front));
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
			cout << "..done" << endl;

			cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << endl;

			cout << "write..";
			write_book(book_name[2], book[2]);
			cout << "..done!" << endl;

		} else if (book_hardfilter || book_softfilter)
		{
			// 一致している定跡の抜き出し
			dMemoryBook book[3];
			string book_name[3];
			is >> book_name[0] >> book_name[1] >> book_name[2];
			if (book_name[2] == "")
			{
				cout << "Error! book name is empty." << endl;
				return;
			}
			cout << "book filter " << book_name[0] << " with " << book_name[1] << " to " << book_name[2] << endl;
			for (int i = 0; i < 2; ++i)
			{
				cout << "reading book" << i << ".";
				if (read_book(book_name[i], book[i]) != 0)
					return;
				cout << ".done." << endl;
			}

			// 読み込めたので抽出する。
			cout << "filter..";

			// 同一nodeと非同一nodeの統計用
			// diffrent_nodes0 = book0側にのみあったnodeの数
			// diffrent_nodes1 = book1側にのみあったnodeの数
			u64 same_nodes = 0;
			u64 diffrent_nodes0 = 0, diffrent_nodes1 = 0;

			// フィルタ
			while (!book[0].book_body.empty() && !book[1].book_body.empty())
			{
				dBookEntry & b0front = book[0].book_body.front();
				dBookEntry & b1front = book[1].book_body.front();
				int cmp = b0front.compare(b1front);
				if (cmp == 0)
				{
					// 同じ局面があったので、book0からbook2に突っ込む。
					same_nodes++;
					dBookEntry be0 = move(b0front);
					dBookEntry be1 = move(b1front);
					book[0].book_body.pop_front();
					book[1].book_body.pop_front();
					if (book_hardfilter) {
						dBookEntry be(be0.sfenPos, (be1.ply > 0 && (be0.ply > be1.ply || be0.ply < 1)) ? be1.ply : be0.ply);
						for (dBookPos bp0 : be0.move_list)
							for (dBookPos bp1 : be1.move_list)
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
					dBookEntry be = move(b0front);
					book[0].book_body.pop_front();
				}
				else
				{
					// book1から読み捨てる
					diffrent_nodes1++;
					dBookEntry be = move(b1front);
					book[1].book_body.pop_front();
				}
			}
			// book0側でまだ突っ込んでいないnodeを読み捨てる
			diffrent_nodes0 += book[0].book_body.size();
			book[0].book_body.clear();
			// book1側でまだ突っ込んでいないnodeを読み捨てる
			diffrent_nodes1 += book[1].book_body.size();
			book[1].book_body.clear();
			cout << "..done" << endl;

			cout << "same nodes = " << same_nodes
				<< " , different nodes =  " << diffrent_nodes0 << " + " << diffrent_nodes1 << endl;

			cout << "write..";
			write_book(book_name[2], book[2]);
			cout << "..done!" << endl;

		} else if (book_sort)
		{
			// 定跡のsort, normalization(n11n)
			dMemoryBook book;
			string book_src, book_dst;
			is >> book_src >> book_dst;
			is_ready();
			cout << "book sort from " << book_src << " , write to " << book_dst << endl;
			if (read_book(book_src, book, false, true) != 0)
				return;

			cout << "write..";
			write_book(book_dst, book);
			cout << "..done!" << endl;

		} else
		{
			cout << "usage" << endl;
			cout << "> bookutil from_sfen book.sfen book.db moves 24" << endl;
			cout << "> bookutil think book.sfen book.db moves 16 depth 18" << endl;
			cout << "> bookutil merge book_src1.db book_src2.db book_merged.db" << endl;
			cout << "> bookutil sort book_src.db book_sorted.db" << endl;
			cout << "> bookutil to_sfen book.db book.sfen moves 256 evaldiff 30 evalblacklimit 0 evalwhitelimit -140 depthlimit 16" << endl;
		}
	}
#endif

	// Move 書き出し
	void movetoa(char ** s, const Move m)
	{
		char * _s = *s;
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
			size_t movelen = str.size();
			char_traits<char>::copy(_s, str.c_str(), movelen);
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

	// dBookEntry 書き出し
	void betoa(char ** s, const dBookEntry & be)
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
		size_t sfenlen = min(be.sfenPos.size(), (size_t)128);
		char_traits<char>::copy(_s, be.sfenPos.c_str(), sfenlen);
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
		else if (c0 == '0' || c0 >= 'a' && c0 <= 'z')
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

	// 定跡ファイルの読み込み(book.db)など。
	int read_book(const string & filename, dMemoryBook & book, bool on_the_fly, bool sfen_n11n)
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

			book.fs.open(filename, ifstream::in);
			if (book.fs.fail())
			{
				cout << "info string Error! : can't read " + filename << endl;
				return 1;
			}

			book.on_the_fly = true;
			book.book_name = filename;
			return 0;
		}

		// 1行ごとの読み込みはオーバーヘッドが大きいので、作業領域を介して読み込む

		// ファイルハンドラ
		ifstream fs;
		
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
			endidx = _idx + (_remain > 1 ? fs.read(buffer + _idx, _remain - 1).gcount() : 0);
			char * _p = buffer + endidx;
			*_p = '\0';
			// 最終行の行頭位置検出
			size_t _lastheadidx = endidx;
			for (; _lastheadidx > 0 && *--_p != '\n'; --_lastheadidx);
			// 全く改行文字が存在しなければ、行末まで読み捨てる
			if (_lastheadidx == 0 && *buffer != '\n')
				fs.ignore(numeric_limits<streamsize>::max(), '\n');
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

		fs.open(filename, ifstream::in | ifstream::binary);
		if (fs.bad() || !fs.is_open())
		{
			cout << "info string Error! : can't read " + filename << endl;
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
			// dBookEntry 構築
			dBookEntry be(buffer + idx0 + 5, idx1 - idx0 - 5, sfen_n11n);
			// dBookEntry への dBookPos要素充填
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
				// dBookEntry に dBookPos 構築 & 追加
				be.move_list.emplace_back(buffer + idx0);
			}
			// おかしければ登録せずに次へ
			if (be.sfenPos.empty() || be.move_list.empty())
				continue;
			// 採択回数でsortしておく
			// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
			be.sort_pos();
			// dMemoryBook に登録
			book.add(be);
		}

		fs.close();

		delete[] buffer;

		// 全体のソート、重複局面の除去
		book.intl_uniq();

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book.book_name = filename;

		return 0;
	}

	// 定跡ファイルの書き出し
	int write_book(const string & filename, dMemoryBook & book)
	{
		// 全体のソートと重複局面の除去
		book.intl_uniq();

		// 1行ごとの書き出しはオーバーヘッドが大きいので、作業領域を介して書き出す

		// 1要素あたりの最大サイズ
		const size_t entrymaxsize = 65536; // 64kiB
		// 領域確保係数
		const size_t renderbufmulti = 256;
		// 最大バッファ領域サイズ
		const size_t renderbufmaxsize = entrymaxsize * renderbufmulti; // 16MiB
		// バッファ領域サイズ
		size_t renderbufsize = entrymaxsize * min(renderbufmulti, book.book_body.size() + 1);
		// バッファ払い出し閾値
		ptrdiff_t renderbufth = (ptrdiff_t)renderbufsize - (ptrdiff_t)entrymaxsize;
		// バッファ確保
		char * renderbuf = new char[renderbufsize];

		// 文字列バッファ char[] に BookEntry を書き出し
		// BookEntry 毎に書き出しを行う
		char * p;

		ofstream fs;
		fs.open(filename, ofstream::out);
		if (fs.bad() || !fs.is_open())
		{
			cout << "info string Error! : can't write " + filename << endl;
			return 1;
		}

		// バージョン識別用文字列
		fs << "#YANEURAOU-DB2016 1.00" << endl;

		*(p = renderbuf) = '\0';
		for (dBookEntry & be : book.book_body)
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

	// char文字列 の dBookPos 化
	void dBookEntry::init(const char * sfen, const size_t length, const bool sfen_n11n)
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
		} else
		{
			auto cur = trimlen_sfen(sfen, length);
			sfenPos = string(sfen, cur);
			ply = QConv::atos32(sfen + cur);
		}
	}

	// ストリームからの dBookEntry 読み込み
	void dBookEntry::init(istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n)
	{
		string line;

		do
		{
			int c;
			// 行頭が's'になるまで読み飛ばす
			while ((c = is.peek()) != 's')
				if (c == EOF || !is.ignore(numeric_limits<streamsize>::max(), '\n'))
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

	// ストリームから dBookEntry への dBookPos 順次読み込み
	void dBookEntry::incpos(istream & is, char * _buffer, const size_t _buffersize)
	{
		while (true)
		{
			int c;
			// 行頭が数字か英文字以外なら行末文字まで読み飛ばす
			while ((c = is.peek(), (c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')))
				if (c == EOF || !is.ignore(numeric_limits<streamsize>::max(), '\n'))
					return;
			// 次のsfen文字列に到達していそうなら離脱（指し手文字列で's'から始まるものは無い）
			if (c == 's')
				return;
			// 行読み込み
			is.getline(_buffer, _buffersize - 8);
			// バッファから溢れたら行末まで読み捨て
			if (is.fail())
			{
				is.ignore(numeric_limits<streamsize>::max(), '\n');
				is.clear(is.rdstate() & ~ios_base::failbit);
			}
			// BookPos 読み込み
			move_list.emplace_back(_buffer);
		}
	}

	// バイト文字列からの dBookPos 読み込み
	void dBookPos::init(const char * p)
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

	vector<BookPos> dMemoryBook::ext_find(const Position & pos)
	{
		// 定跡がないならこのまま返る。(sfen()を呼び出すコストの節約)
		if (!on_the_fly && book_body.size() == 0)
			return vector<BookPos>();

		if (on_the_fly)
		{
			// ディスクから読み込むなら、いずれにせよ、extl_book_bodyをクリアして、
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
			fs.seekg(0, ios::end);
			auto file_end = fs.tellg();

			fs.clear();
			fs.seekg(0, ios::beg);
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
					// "sfen "という文字列は取り除いたものを返す。
					// 手数の表記も取り除いて比較したほうがいい。
					// ios::binaryつけているので末尾に'\R'が付与されている。禿げそう。
					if (!line.compare(0, 5, "sfen "))
						return trim_sfen(line.substr(5));
				return string();
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
						return vector<BookPos>();

					// もしかしたら先頭付近にあるかも知れん..
					e = 0; // この条件で再度サーチ
				}

			}
			// 見つけた処理

			// 行バッファ確保
			const size_t _buffersize = 256;
			char _buffer[_buffersize];

			dBookEntry be(sfen, pos.game_ply());
			be.incpos(fs, _buffer, _buffersize);
			be.sort_pos();

			vector<BookPos> mlist = be.export_move_list();

			// 定跡のMoveは16bitであり、rootMovesは32bitのMoveであるからこのタイミングで補正する。
			for (auto & m : mlist)
				m.bestMove = pos.move16_to_move(m.bestMove);

			return mlist;

		} else
		{
			// on the flyではない場合
			BookType::iterator it = find(pos);
			if (it != end())
			{
				vector<BookPos> mlist = it->export_move_list();

				// 定跡のMoveは16bitであり、rootMovesは32bitのMoveであるからこのタイミングで補正する。
				for (auto & m : mlist)
					m.bestMove = pos.move16_to_move(m.bestMove);

				return mlist;
			} else
				return vector<BookPos>();
		}

	}

	// 出力ストリーム
	// write_book では最適化のため現在使われていない
	ostream & operator << (ostream & os, const dBookPos & bp)
	{
		os << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << ' ' << bp.depth << ' ' << bp.num << "\n";
		return os;
	}
	ostream & operator << (ostream & os, const dBookEntry & be)
	{
		os << "sfen " << be.sfenPos << " " << be.ply << "\n";
		for (auto & bp : be.move_list)
			os << bp;
		return os;
	}

}
