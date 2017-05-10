#ifndef _BOOK_UTIL_H_
#define _BOOK_UTIL_H_

#include "book.h"
#include <algorithm>
#include <deque>
#include <iterator>
#include <utility>

namespace Book {

	typedef typename std::pair<std::string, int> sfen_pair_t;
	typedef typename std::allocator<char> alloc_char_t;
	typedef typename std::basic_string<char, std::char_traits<char>, alloc_char_t> dsfen_t;

	// 手数を除いた平手初期局面のsfen文字列
	const std::string SHORTSFEN_HIRATE = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -";
	std::size_t trimlen_sfen(const char * sfen, const size_t length);
	std::size_t trimlen_sfen(const std::string sfen);
	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const std::string sfen);

	// 局面における指し手(定跡を格納するのに用いる)
	struct dBookPos
	{

		Move bestMove;
		Move nextMove;
		int value;
		int depth;
		u64 num;

		dBookPos(const dBookPos & bp) : bestMove(bp.bestMove), nextMove(bp.nextMove), value(bp.value), depth(bp.depth), num(bp.num) {}
		dBookPos(Move best, Move next, int v, int d, u64 n) : bestMove(best), nextMove(next), value(v), depth(d), num(n) {}

		// char文字列からの dBookPos生成
		void init(const char * p);
		dBookPos(const char * p) { init(p); }

		bool operator == (const dBookPos & rhs) const
		{
			return bestMove == rhs.bestMove;
		}
		bool operator < (const dBookPos & rhs) const
		{
			// std::sortで降順ソートされて欲しいのでこう定義する。
			return value > rhs.value || value == rhs.value && num > rhs.num;
		}

	};

	typedef typename dBookPos dbp_t;
	typedef typename std::allocator<dbp_t> alloc_dbp_t;
	typedef typename std::vector<dBookPos, alloc_dbp_t> ctr_dbp_t;
	typedef typename ctr_dbp_t::iterator it_dbp_t;
	typedef typename std::iterator_traits<it_dbp_t>::difference_type itdiff_ctr_dbp_t;

	// 内部実装用局面
	struct dSfenPos
	{
		dsfen_t sfenPos;
		int ply;

		dSfenPos() : sfenPos(SHORTSFEN_HIRATE), ply(1) {}
		dSfenPos(const std::string sfen, const int p) : sfenPos(sfen), ply(p) {}
		dSfenPos(const sfen_pair_t & sfen_pair) : sfenPos(sfen_pair.first), ply(sfen_pair.second) {}
		dSfenPos(const Position & pos) : sfenPos(pos.trimedsfen()), ply(pos.game_ply()) {}
		dSfenPos(const dSfenPos & pos) : sfenPos(pos.sfenPos), ply(pos.ply) {}

		// 局面比較
		bool operator < (const dSfenPos & rhs) const
		{
			return sfenPos < rhs.sfenPos;
		}
		bool operator == (const dSfenPos & rhs) const
		{
			return sfenPos == rhs.sfenPos;
		}
		int compare(const dSfenPos & rhs) const
		{
			return sfenPos.compare(rhs.sfenPos);
		}
	};

	// 内部実装用局面項目
	struct dBookEntry : dSfenPos
	{
		ctr_dbp_t move_list;

		dBookEntry() : dSfenPos(), move_list() {}
		dBookEntry(const std::string sfen, const int p) : dSfenPos(sfen, p), move_list() {}
		dBookEntry(const sfen_pair_t & sfen_pair) : dSfenPos(sfen_pair), move_list() {}
		dBookEntry(const sfen_pair_t & sfen_pair, ctr_dbp_t mlist) : dSfenPos(sfen_pair), move_list(mlist) {}
		dBookEntry(const Position & pos) : dSfenPos(pos), move_list() {}

		// char文字列からの dBookEntry 生成
		void init(const char * sfen, const size_t length, const bool sfen_n11n);
		dBookEntry(const char * sfen, const size_t length, const bool sfen_n11n = false) { init(sfen, length, sfen_n11n); }

		// ストリームからの dBookPos 順次読み込み
		void incpos(std::istream & is, char * _buffer, const size_t _buffersize);

		// ストリームからの dBookEntry 読み込み
		void init(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n);
		dBookEntry(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n = false)
		{
			init(is, _buffer, _buffersize, sfen_n11n);
		}

		// 局面比較
		bool operator < (const dBookEntry & rhs) const
		{
			return sfenPos < rhs.sfenPos;
		}
		bool operator == (const dBookEntry & rhs) const
		{
			return sfenPos == rhs.sfenPos;
		}
		int compare(const dBookEntry & rhs) const
		{
			return sfenPos.compare(rhs.sfenPos);
		}

		// 優位な局面情報の選択
		void select(const dBookEntry & be)
		{
			// 異なる局面は処理しない
			if (sfenPos != be.sfenPos)
				return;

			// 手数の少ない方に合わせる
			if (be.ply > 0 && (ply > be.ply || ply < 1))
				ply = be.ply;

			if (
				// 空ではない方
				!be.move_list.empty() && (move_list.empty() ||
					// depth が深い方
					move_list.front().depth < be.move_list.front().depth ||
					// depth が同じなら MultiPV の多い方、それでも同じなら後者
					move_list.front().depth == be.move_list.front().depth &&
					move_list.size() < be.move_list.size()
					)
				)
				move_list = be.move_list;

		}

		void sort_pos()
		{
			std::stable_sort(move_list.begin(), move_list.end());
		}

		void insert_book_pos(const dBookPos & bp)
		{
			for (auto & b : move_list)
			{
				if (b == bp)
				{
					auto num = b.num + bp.num;
					b = bp;
					b.num = num;
					return;
				}
			}
			move_list.push_back(bp);
		}

		std::vector<BookPos> export_move_list()
		{
			std::vector<BookPos> mlist;
			// vector<dBookPos> から vector<BookPos> へのコピー
			u64 num_sum = 0;
			for (auto & bp : move_list)
			{
				mlist.emplace_back(bp.bestMove, bp.nextMove, bp.value, bp.depth, bp.num);
				num_sum += bp.num;
			}
			// 候補手のソート
			std::stable_sort(mlist.begin(), mlist.end());
			// 候補手の選択率算出
			for (auto & bp : mlist)
				bp.prob = float(bp.num) / num_sum;
			return mlist;
		}

	};

	struct dMemoryBook
	{
		typedef dBookEntry dbe_t; 
		typedef std::deque<dbe_t> BookType;
		typedef typename BookType::iterator it_dbe_t;
		typedef typename std::iterator_traits<it_dbe_t>::difference_type diff_dbe_t;
		typedef std::vector<diff_dbe_t> BookRun;

		// 定跡本体
		// dMemoryBook 同士のマージを行いやすくするため、
		BookType book_body;

		// ソート済み区間の保持
		// book_body をマージソートの中間状態とみなし、ソート済み区間の区切り位置を保持する
		// これが空の場合、 book_body は全てソート済みであるとみなす
		BookRun book_run;

		// 読み込んだbookの名前
		std::string book_name;

		// メモリに丸読みせずにfind()ごとにファイルを調べにいくのか。
		bool on_the_fly = false;

		// 上の on_the_fly == true の時に、開いている定跡ファイルのファイルハンドル
		std::ifstream fs;

		// 挿入ソートを行うしきい値
		const std::size_t MINRUN = 32;

		// 比較関数
		struct less_bebe
		{
			bool operator()(const dbe_t & lhs, const dbe_t & rhs) const
			{
				return bool(lhs.sfenPos < rhs.sfenPos);
			}
		};
		struct less_bestr
		{
			bool operator()(const dbe_t & lhs, const std::string & rhs) const
			{
				return bool(lhs.sfenPos < rhs);
			}
		};

		// find() で見つからなかった時の値
		const it_dbe_t end()
		{
			return book_body.end();
		}

		it_dbe_t find(const dbe_t & be)
		{
			auto it0 = book_body.begin();
			auto itr = book_run.begin();
			while (true)
			{
				auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
				auto itf = std::lower_bound(it0, it1, be, less_bebe());
				if (itf != it1 && *itf == be)
					return itf;
				if (itr == book_run.end())
					return end();
				++itr;
				it0 = it1;
			}
		}
		it_dbe_t find(const std::string & sfenPos)
		{
			auto it0 = book_body.begin();
			auto itr = book_run.begin();
			while (true)
			{
				auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
				auto itf = std::lower_bound(it0, it1, sfenPos, less_bestr());
				if (itf != it1 && (*itf).sfenPos == sfenPos)
					return itf;
				if (itr == book_run.end())
					return end();
				++itr;
				it0 = it1;
			}
		}
		it_dbe_t find(const Position & pos)
		{
			return find(pos.trimedsfen());
		}

		std::vector<BookPos> ext_find(const Position & pos);
		
		// 部分的マージソート
		void intl_merge_part()
		{
			bool f = !book_run.empty();
			auto end_d = std::distance(book_body.begin(), book_body.end());
			while (f)
			{
				f = false;
				auto run_size = book_run.size();
				diff_dbe_t run_back2, run_back;
				it_dbe_t body_begin;
				switch (run_size)
				{
				case 1:
					run_back = book_run.back();
					if (run_back <= end_d - run_back)
					{
						body_begin = book_body.begin();
						std::inplace_merge(body_begin, std::next(body_begin, run_back), book_body.end());
						book_run.pop_back();
					}
					break;
				case 2:
					run_back2 = book_run.front();
					run_back = book_run.back();
					if (run_back - run_back2 <= end_d - run_back)
					{
						body_begin = book_body.begin();
						std::inplace_merge(std::next(body_begin, run_back2), std::next(body_begin, run_back), book_body.end());
						book_run.pop_back();
						f = true;
					} else if (run_back2 <= end_d - run_back2)
					{
						body_begin = book_body.begin();
						std::inplace_merge(std::next(body_begin, run_back2), std::next(body_begin, run_back), book_body.end());
						book_run.pop_back();
						body_begin = book_body.begin();
						std::inplace_merge(body_begin, std::next(body_begin, run_back2), book_body.end());
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
						std::inplace_merge(std::next(body_begin, run_back2), std::next(body_begin, run_back), book_body.end());
						book_run.pop_back();
						f = true;
					}
				}
			}
		}

		// マージソート(全体)
		void intl_merge()
		{
			while (!book_run.empty())
			{
				diff_dbe_t d1 = book_run.back();
				book_run.pop_back();
				diff_dbe_t d0 = book_run.empty() ? (diff_dbe_t)0 : book_run.back();
				auto body_begin = book_body.begin();
				std::inplace_merge(std::next(body_begin, d0), std::next(body_begin, d1), book_body.end());
			}
		}

		// 同一局面の整理
		// 大量に局面登録する際、毎度重複チェックを行うよりまとめて処理する方が？
		void intl_uniq()
		{
			// あらかじめ全体をソートしておく
			intl_merge();

			if (book_body.empty()) return;
			it_dbe_t it0 = book_body.begin(), it1 = std::next(it0), end = book_body.end();
			while (it1 != end)
				if (*it0 == *it1)
				{
					do
						(*it0).select(std::move(*it1++));
					while (it1 != end && *it0 == *it1);
					it0 = book_body.erase(++it0, it1);
					end = book_body.end();
					it1 = std::next(it0);
				}
				else
				{
					++it0;
					++it1;
				}
		}

		// 局面の追加
		// 大量の局面を追加する場合、重複チェックを逐一は行わず(dofind_false)に、後で intl_uniq() を行うことを推奨
		void add(dbe_t & be, bool dofind = false)
		{
			it_dbe_t it;
			int cmp;
			if (dofind && (it = find(be)) != end())
				// 重複していたら整理して再登録
				(*it).select(be);
			else if (book_body.empty() || (cmp = be.compare(book_body.back())) > 0)
				// 順序関係が保たれているなら単純に末尾に追加
				book_body.push_back(be);
			else if (cmp == 0)
				// 末尾と同一局面なら整理
				book_body.back().select(be);
			else if (book_run.empty())
				if (book_body.size() < MINRUN)
					// 短ければ、挿入ソート
					book_body.insert(std::upper_bound(book_body.begin(), book_body.end(), be), be);
				else
				{
					// 長ければ、新しいソート済み範囲を追加
					book_run.push_back(std::distance(book_body.begin(), book_body.end()));
					book_body.push_back(be);
				}
			else
			{
				if (std::distance(std::next(book_body.begin(), book_run.back()), book_body.end()) < (diff_dbe_t)MINRUN)
					// 短ければ、挿入ソート
					book_body.insert(std::upper_bound(std::next(book_body.begin(), book_run.back()), book_body.end(), be), be);
				else
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
		void insert_book_pos(const sfen_pair_t & sfen, dBookPos & bp)
		{
			auto it = find(sfen.first);
			if (it == end())
			{
				// 存在しないので要素を作って追加。
				dbe_t be(sfen);
				be.move_list.push_back(bp);
				add(be);
			} else
				// この局面での指し手のリスト
				it ->insert_book_pos(bp);
		}

		void clear()
		{
			book_body.clear();
			book_run.clear();
		}

	};

	// 出力ストリーム
	std::ostream & operator << (std::ostream & os, const dBookPos & bp);
	std::ostream & operator << (std::ostream & os, const dBookEntry & be);

	// 定跡ファイルの読み込み(book.db)など。
	// 同じファイルを二度目は読み込み動作をskipする。
	// on_the_flyが指定されているとメモリに丸読みしない。
	// 定跡作成時などはこれをtrueにしてはいけない。（メモリに読み込まれないため）
	extern int read_book(const std::string & filename, dMemoryBook & book, bool on_the_fly = false, bool sfen_n11n = false);

	// 定跡ファイルの書き出し
	extern int write_book(const std::string & filename, dMemoryBook & book);

#ifdef ENABLE_MAKEBOOK_CMD
	extern void bookutil_cmd(Position & pos, std::istringstream & is);
#endif

}

#endif // #ifndef _BOOK_UTIL_H_
