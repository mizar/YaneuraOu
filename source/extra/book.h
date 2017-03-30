#ifndef _BOOK_H_
#define _BOOK_H_

#include "../shogi.h"
#include "../position.h"
#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>

// 定跡処理関係
namespace Book
{
	// 手数を除いた平手初期局面のsfen文字列
	const std::string SHORTSFEN_HIRATE = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -";
	// sfen文字列のゴミを除いた長さ
	const std::size_t trimlen_sfen(const char* sfen, const size_t length);
	const std::size_t trimlen_sfen(const std::string sfen);
	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const std::string trim_sfen(const std::string sfen);
	// sfen文字列の手数分離
	const std::pair<const std::string, const int> split_sfen(const std::string sfen);

	// 局面における指し手(定跡を格納するのに用いる)
	struct BookPos
	{
		Move bestMove; // この局面での指し手
		Move nextMove; // その指し手を指したときの予想される相手の指し手
		int value;     // bestMoveを指したときの局面の評価値
		int depth;     // bestMoveの探索深さ
		u64 num;       // 何らかの棋譜集において、この指し手が採択された回数。
		float prob;    // ↑のnumをパーセンテージで表現したもの。(read_bookしたときには反映される。ファイルには書き出していない。)

		BookPos(Move best, Move next, int v, int d, u64 n) : bestMove(best), nextMove(next), value(v), depth(d), num(n) {}

		// char文字列からの BookPos 生成
		void set(const char* p);
		BookPos(const char* p) { set(p); }

		// 比較
		bool operator == (const BookPos& rhs) const { return bestMove == rhs.bestMove; }
		bool operator < (const BookPos& rhs) const { return value > rhs.value || value == rhs.value && num > rhs.num; } // std::sortで降順ソートされて欲しいのでこう定義する。

	};

	// 局面ごとの項目
	struct BookEntry
	{
		std::string sfenPos; // 末尾の手数を除くsfen局面文字列
		int ply; // 手数
		std::vector<BookPos> move_list; // 候補手リスト

		BookEntry() : sfenPos(SHORTSFEN_HIRATE), ply(1), move_list() {}
		BookEntry(const std::string sfen, const int p) : sfenPos(sfen), ply(p), move_list() {}
		BookEntry(const std::string sfen, const int p, std::vector<BookPos> mlist) : sfenPos(sfen), ply(p), move_list(mlist) {}
		BookEntry(const std::pair<const std::string, const int> sfen_pair) : sfenPos(sfen_pair.first), ply(sfen_pair.second), move_list() {}
		BookEntry(const std::pair<const std::string, const int> sfen_pair, std::vector<BookPos> mlist) : sfenPos(sfen_pair.first), ply(sfen_pair.second), move_list(mlist) {}
		BookEntry(const Position& pos) : sfenPos(pos.trimedsfen()), ply(pos.game_ply()), move_list() {}
		BookEntry(const Position& pos, std::vector<BookPos> mlist) : sfenPos(pos.trimedsfen()), ply(pos.game_ply()), move_list(mlist) {}

		// char文字列からの BookEntry 生成
		void set(const char* sfen, const size_t length, const bool sfen_n11n);
		BookEntry(const char* sfen, const size_t length, const bool sfen_n11n = false) { set(sfen, length, sfen_n11n); }

		// ストリームからの BookPos 順次読み込み
		void incpos(std::istream& is, char* _buffer, const size_t _buffersize);

		// ストリームからの BookEntry 読み込み
		void set(std::istream& is, char* _buffer, const size_t _buffersize, const bool sfen_n11n);
		BookEntry(std::istream& is, char* _buffer, const size_t _buffersize, const bool sfen_n11n = false) { set(is, _buffer, _buffersize, sfen_n11n); }

		// 局面比較
		bool operator < (const BookEntry& rhs) const { return sfenPos < rhs.sfenPos; }
		bool operator == (const BookEntry& rhs) const { return sfenPos == rhs.sfenPos; }
		int compare(const BookEntry& rhs) const { return sfenPos.compare(rhs.sfenPos); }

		// 同一局面の合成
		void update(const BookEntry& be)
		{
			// 同一局面のみ合成
			if (sfenPos == be.sfenPos)
			{
				// 手数の少ない方に合わせる
				if (be.ply > 0 && (ply > be.ply || ply < 1))
					ply = be.ply;
				if (
					// 空では無い方
					!be.move_list.empty() && (move_list.empty() ||
					// depthが深い方
					move_list[0].depth < be.move_list[0].depth ||
					// depthが同じならMultiPVの多い方、それでも同じなら後者
					move_list[0].depth == be.move_list[0].depth &&
					move_list.size() <= be.move_list.size()
					))
					move_list = be.move_list;
			}
		}

		// 候補手のソート
		void sort_pos() { std::stable_sort(move_list.begin(), move_list.end()); }

		// 候補手の追加
		void insert_book_pos(const BookPos& bp)
		{
			for (auto& b : move_list)
				if (b == bp)
				{
					auto num = b.num + bp.num;
					b = bp;
					b.num = num;
					return;
				}
			// move_listがソート済みであると仮定して挿入
			move_list.insert(std::upper_bound(move_list.begin(), move_list.end(), bp), bp);
		}

		// 候補手の選択率算出
		void calc_prob()
		{
			std::stable_sort(move_list.begin(), move_list.end());
			u64 num_sum = 0;
			for (auto& bp : move_list)
				num_sum += bp.num;
			for (auto& bp : move_list)
				bp.prob = float(bp.num) / num_sum;      
		}

	};

	// メモリ上にある定跡ファイル
	// sfen文字列をkeyとして、局面の指し手へ変換。(重複した指し手は除外するものとする)
	struct MemoryBook
	{
		typedef typename std::deque<BookEntry>::iterator iter_t;
		typedef typename std::iterator_traits<iter_t>::difference_type diff_t;
		typedef std::deque<BookEntry> BookType;
		typedef std::vector<diff_t> BookRun;

		// 定跡本体
		// MemoryBook同士のマージを行いやすくするため、vectorではなくdequeで実装
		BookType book_body;

		// ソート済み区間の保持
		// book_bodyをマージソートの中間状態と見なし、ソート済み区間の区切り位置を保持する
		// これが空の場合、book_bodyは全てソート済みであると見なす
		BookRun book_run;

		// 読み込んだbookの名前
		std::string book_name;

		// メモリに丸読みせずにfind()のごとにファイルを調べにいくのか。
		bool on_the_fly = false;

		// 上のon_the_fly == trueのときに、開いている定跡ファイルのファイルハンドル
		std::fstream fs;

		// 挿入ソートを行うしきい値
		const std::size_t MINRUN = 32;

		// find()で見つからなかったときの値
		const iter_t end() { return book_body.end(); }

		// book_body内の定跡を探索
		iter_t intl_find(BookEntry& be)
		{
			auto it0 = book_body.begin();
			auto itr = book_run.begin();
			while (true)
			{
				auto it1 = (itr != book_run.end()) ? book_body.begin() + *itr : book_body.end();
				auto itf = std::lower_bound(it0, it1, be);
				if (itf != it1 && *itf == be)
					return itf;
				if (itr == book_run.end())
					return end();
				itr++;
				it0 = it1;
			}
		}

		iter_t intl_find(Position pos)
		{
			BookEntry be(pos);
			return intl_find(be);
		}

		iter_t intl_find(std::string sfen)
		{
			BookEntry be(split_sfen(sfen));
			return intl_find(be);
		}

		// 定跡として登録されているかを調べて返す。
		// readのときにon_the_flyが指定されていればファイルを調べに行く。
		// (このとき、見つからなければthis::end()が返ってくる。
		// ファイルに読み込みに行っていることを意識する必要はない。)
		iter_t find(const Position& pos);

		// BookRun再構築
		// 外部でbook_bodyに破壊的な操作を行った後に実行する
		void run_rebuild()
		{
			book_run.clear();
			auto first = book_body.begin(), mid = first, last = book_body.end();
			while ((mid = std::is_sorted_until(mid, last)) != last)
				book_run.push_back(std::distance(first, mid));
		}

		// マージソート(部分的)
		// length[n-1] > length[n] && length[n-2] > (length[n-1] + length[n]) の状態を満たすまでマージソートを行う。
		void intl_merge_part()
		{
			bool f = !book_run.empty();
			auto end_d = std::distance(book_body.begin(), book_body.end());
			while (f)
			{
				f = false;
				auto run_size = book_run.size();
				switch (run_size)
				{
				case 1:
					if (book_run.front() <= end_d - book_run.front())
					{
						std::inplace_merge(book_body.begin(), book_body.begin() + book_run.front(), book_body.end());
						book_run.pop_back();
					}
					break;
				case 2:
					if (book_run.back() - book_run.front() <= end_d - book_run.back())
					{
						std::inplace_merge(book_body.begin() + book_run.front(), book_body.begin() + book_run.back(), book_body.end());
						book_run.pop_back();
						f = true;
					} else if (book_run.front() <= end_d - book_run.front())
					{
						std::inplace_merge(book_body.begin() + book_run.front(), book_body.begin() + book_run.back(), book_body.end());
						book_run.pop_back();
						std::inplace_merge(book_body.begin(), book_body.begin() + book_run.front(), book_body.end());
						book_run.pop_back();
					}
					break;
				default:
					if (
						book_run.back() - book_run[run_size - 2] <= end_d - book_run.back() ||
						book_run[run_size - 2] - book_run[run_size - 3] <= end_d - book_run[run_size - 2]
					)
					{
						std::inplace_merge(
							book_body.begin() + book_run[run_size - 2],
							book_body.begin() + book_run.back(),
							book_body.end()
						);
						book_run.pop_back();
						f = true;
					}
					break;
				}
			}
		}

		// マージソート(全体)
		void intl_merge()
		{
			while (!book_run.empty())
			{
				auto d1 = book_run.back();
				book_run.pop_back();
				auto d0 = book_run.empty() ? std::distance(book_body.begin(), book_body.begin()) : book_run.back();
				std::inplace_merge(book_body.begin() + d0, book_body.begin() + d1, book_body.end());
			}
		}

		// 同一局面の整理
		// 大量に局面登録する際、毎度重複チェックを行うと計算量が莫大になるのでまとめて処理
		void intl_uniq()
		{
			// 予め全体をソートしておく
			intl_merge();

			if (book_body.empty()) return;
			std::size_t i = 1;
			iter_t it0 = book_body.begin(), it1 = it0 + 1, end = book_body.end();
			while (it1 != end)
				if (*it0 == *it1)
				{
					do {
						(*it0).update(std::move(*it1++));
					} while (it1 != end && *it0 == *it1);
					it0 = book_body.erase(++it0, it1);
					end = book_body.end();
					if (it0 == end) return;
					it1 = it0 + 1;
				}
				else
				{
					++it0;
					++it1;
					++i;
				}
		}

		// 局面の追加
		// 大量の局面を追加する場合、重複チェックは逐一行わず(dofind=false)に、後でintl_uniq()を行う事を推奨
		void add(BookEntry& be, bool dofind = false)
		{
			iter_t it;
			int cmp;
			if (dofind && (it = intl_find(be)) != end())
			{
				// 重複していたら合成して再登録
				(*it).update(be);
			} else if (book_body.empty() || (cmp = be.compare(book_body.back())) > 0)
			{
				// 順序関係が保たれているなら単純に末尾に追加
				book_body.push_back(be);
			} else if (cmp == 0)
			{
				// 末尾と同一局面なら合成
				book_body.back().update(be);
			} else if (book_run.empty())
			{
				// 既に全体がソート済みの場合、全体の長さを見て
				if (book_body.size() < MINRUN)
				{
					// 短ければ、挿入ソート
					book_body.insert(upper_bound(book_body.begin(), book_body.end(), be), be);
				} else
				{
					// 長ければ、新しいソート済み範囲を追加
					book_run.push_back(std::distance(book_body.begin(), book_body.end()));
					book_body.push_back(be);
				}
			} else
			{
				// ソート済み範囲が区分化されている場合、末尾の範囲の長さを見て
				if (std::distance(book_body.begin() + book_run.back(), book_body.end()) < (diff_t)MINRUN)
				{
					// 短ければ、挿入ソート
					book_body.insert(std::upper_bound(book_body.begin() + book_run.back(), book_body.end(), be), be);
				} else
				{
					// 長ければ、部分的なマージソートを試みてから
					intl_merge_part();
					// 新しいソート済み範囲を追加
					book_run.push_back(std::distance(book_body.begin(), book_body.end()));
					book_body.push_back(be);
				}
			}
		}

		// 局面・指し手の追加
		void insert_book_pos(std::string sfen, BookPos& bp)
		{
			auto it = intl_find(sfen);
			if (it == end())
			{
				// 存在しないので要素を作って追加。
				std::vector<BookPos> move_list{ bp };
				BookEntry be(split_sfen(sfen), move_list);
				add(be);
			} else
			{
				// この局面での指し手のリスト
				it->insert_book_pos(bp);
			}
		}

		// 定跡のクリア
		void clear()
		{
			book_body.clear();
			book_run.clear();
		}

	};

	// 出力ストリーム
	std::ostream& operator << (std::ostream& os, const BookPos& bp);
	std::ostream& operator << (std::ostream& os, const BookEntry& be);

	// 定跡ファイルの読み込み(book.db)など。
	// 同じファイルを二度目は読み込み動作をskipする。
	// on_the_flyが指定されているとメモリに丸読みしない。
	// 定跡作成時などはこれをtrueにしてはいけない。(メモリに読み込まれないため)
	extern int read_book(const std::string& filename, MemoryBook& book, bool on_the_fly = false, bool sfen_n11n = false);

	// 定跡ファイルの書き出し
	extern int write_book(const std::string& filename, MemoryBook& book);

#ifdef ENABLE_MAKEBOOK_CMD
	// USI拡張コマンド。"makebook"。定跡ファイルを作成する。
	// フォーマット等についてはdoc/解説.txt を見ること。
	extern void makebook_cmd(Position& pos, std::istringstream& is);
#endif


}

#endif // #ifndef _BOOK_H_
