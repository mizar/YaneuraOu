#ifndef _BOOK_H_
#define _BOOK_H_

#include "../shogi.h"
#include "../position.h"
#include <algorithm>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <scoped_allocator>
#include <sstream>
#include <utility>

// 定跡処理関係
namespace Book
{
	// 手数を除いた平手初期局面のsfen文字列
	const std::string SHORTSFEN_HIRATE = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -";
	// sfen文字列のゴミを除いた長さ
	std::size_t trimlen_sfen(const char * sfen, const size_t length);
	std::size_t trimlen_sfen(const std::string sfen);
	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const std::string trim_sfen(const std::string sfen);
	// sfen文字列の手数分離
	const std::pair<const std::string, const int> split_sfen(const std::string sfen);

	// 局面における指し手(定跡を格納するのに用いる)
	struct IntlBookPos
	{

		Move bestMove; // この局面での指し手
		Move nextMove; // その指し手を指したときの予想される相手の指し手
		int value;     // bestMoveを指したときの局面の評価値
		int depth;     // bestMoveの探索深さ
		u64 num;       // 何らかの棋譜集において、この指し手が採択された回数。

		IntlBookPos(const IntlBookPos & bp) : bestMove(bp.bestMove), nextMove(bp.nextMove), value(bp.value), depth(bp.depth), num(bp.num) {}
		IntlBookPos(Move best, Move next, int v, int d, u64 n) : bestMove(best), nextMove(next), value(v), depth(d), num(n) {}

		// char文字列からの InternalBookPos 生成
		void set(const char * p);
		IntlBookPos(const char * p) { set(p); }

		// 比較
		bool operator == (const IntlBookPos & rhs) const { return bestMove == rhs.bestMove; }
		bool operator < (const IntlBookPos & rhs) const { return value > rhs.value || value == rhs.value && num > rhs.num; } // std::sortで降順ソートされて欲しいのでこう定義する。

	};

	// 局面における指し手(外部実装の互換用)
	struct BookPos : IntlBookPos
	{
		float prob;    // IntlBookPosのnumをパーセンテージで表現したもの。(findしたときには反映される。ファイルには書き出していない。)

		BookPos(const IntlBookPos & bp, float p = 0) : IntlBookPos(bp), prob(p) {}
		BookPos(Move best, Move next, int v, int d, u64 n, float p = 0) : IntlBookPos(best, next, v, d, n), prob(p) {}
		// 比較
		bool operator == (const BookPos & rhs) const { return bestMove == rhs.bestMove; }
		bool operator < (const BookPos & rhs) const { return value > rhs.value || value == rhs.value && num > rhs.num; } // std::sortで降順ソートされて欲しいのでこう定義する。
	};

	typedef typename std::allocator<IntlBookPos> ialloc_bp_t;
	typedef typename std::allocator<char> ialloc_char_t;
	typedef typename std::basic_string<char, std::char_traits<char>, ialloc_char_t> isfenstr_t;
	typedef typename std::forward_list<IntlBookPos, ialloc_bp_t> cibp_t;
	typedef typename std::iterator_traits<cibp_t::iterator>::difference_type itdiff_cibp_t;

	// 局面
	struct SfenPos
	{
		isfenstr_t sfenPos;
		int ply;

		SfenPos() : sfenPos(SHORTSFEN_HIRATE), ply(1) {}
		SfenPos(const std::string sfen, const int p) : sfenPos(sfen), ply(p) {}
		SfenPos(const std::pair <const std::string, const int> & sfen_pair) : sfenPos(sfen_pair.first), ply(sfen_pair.second) {}
		SfenPos(const Position & pos) : sfenPos(pos.trimedsfen()), ply(pos.game_ply()) {}

		// 局面比較
		bool operator < (const SfenPos & rhs) const { return sfenPos < rhs.sfenPos; }
		bool operator == (const SfenPos & rhs) const { return sfenPos == rhs.sfenPos; }
		int compare(const SfenPos & rhs) const { return sfenPos.compare(rhs.sfenPos); }
	};

	// 局面ごとの項目（内部実装用）
	struct IntlBookEntry : SfenPos
	{

		cibp_t move_list; // 候補手リスト

		IntlBookEntry() : SfenPos(), move_list() {}
		IntlBookEntry(const std::string sfen, const int p) : SfenPos(sfen, p), move_list() {}
		IntlBookEntry(const std::pair<const std::string, const int> & sfen_pair) : SfenPos(sfen_pair), move_list() {}
		IntlBookEntry(const std::pair<const std::string, const int> & sfen_pair, std::forward_list<IntlBookPos> mlist) : SfenPos(sfen_pair), move_list(mlist) {}
		IntlBookEntry(const Position & pos) : SfenPos(pos), move_list() {}

		ptrdiff_t move_list_size() const { return std::distance(move_list.cbegin(), move_list.cend()); }

		// char文字列からの IntlBookEntry 生成
		void set(const char * sfen, const size_t length, const bool sfen_n11n);
		IntlBookEntry(const char * sfen, const size_t length, const bool sfen_n11n = false) { set(sfen, length, sfen_n11n); }

		// ストリームからの IntlBookPos 順次読み込み
		void incpos(std::istream & is, char * _buffer, const size_t _buffersize);

		// ストリームからの IntlBookEntry 読み込み
		void set(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n);
		IntlBookEntry(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n = false) { set(is, _buffer, _buffersize, sfen_n11n); }

		// 局面比較
		bool operator < (const IntlBookEntry & rhs) const { return sfenPos < rhs.sfenPos; }
		bool operator == (const IntlBookEntry & rhs) const { return sfenPos == rhs.sfenPos; }
		int compare(const IntlBookEntry & rhs) const { return sfenPos.compare(rhs.sfenPos); }

		// 同一局面の合成
		void update(const IntlBookEntry & be)
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
					move_list.front().depth < be.move_list.front().depth ||
					// depthが同じならMultiPVの多い方、それでも同じなら後者
					move_list.front().depth == be.move_list.front().depth &&
					move_list_size() <= be.move_list_size()
					))
					move_list = be.move_list;
			}
		}

		// 候補手のソート
		void sort_pos() { move_list.sort(); }

		// 候補手の追加
		void insert_book_pos(const IntlBookPos & bp)
		{
			for (auto & b : move_list)
				if (b == bp)
				{
					auto num = b.num + bp.num;
					b = bp;
					b.num = num;
					return;
				}
			move_list.push_front(bp);
		}

	};

	// 局面の項目（外部実装の互換用）
	struct BookEntry : SfenPos
	{
		std::vector<BookPos> move_list;

		BookEntry(IntlBookEntry & be) : SfenPos(be.sfenPos, be.ply)
		{
			// forward_list<IntlBookPos> から vector<BookPos> へのコピー
			std::for_each(be.move_list.begin(), be.move_list.end(), [&](IntlBookPos & bp) { move_list.emplace_back(bp); });
			// 候補手のソート
			std::stable_sort(move_list.begin(), move_list.end());
			// 候補手の選択率算出
			u64 num_sum = 0;
			for (auto & bp : move_list)
				num_sum += bp.num;
			for (auto & bp : move_list)
				bp.prob = float(bp.num) / num_sum;
		}

	};

	typedef typename IntlBookEntry ibe_t;
	typedef typename std::scoped_allocator_adaptor<std::allocator<ibe_t>, ialloc_char_t, ialloc_bp_t> ialloc_cbe_t;

	// メモリ上にある定跡ファイル
	// sfen文字列をkeyとして、局面の指し手へ変換。(重複した指し手は除外するものとする)
	struct MemoryBook
	{
		typedef std::deque<ibe_t, ialloc_cbe_t> IntlBookType;
		typedef typename IntlBookType::iterator it_ibe_t;
		typedef typename std::iterator_traits<it_ibe_t>::difference_type intl_diff_t;
		typedef std::vector<intl_diff_t> IntlBookRun;
		typedef std::deque<BookEntry> ExtlBookType;
		typedef typename ExtlBookType::iterator extl_iter_t;
		typedef typename std::iterator_traits<it_ibe_t>::difference_type extl_diff_t;

		ialloc_cbe_t alloc;

		// 定跡本体
		// MemoryBook同士のマージを行いやすくするため、vectorではなくdequeで実装
		IntlBookType intl_book_body;
		ExtlBookType extl_book_body;

		// ソート済み区間の保持
		// intl_book_bodyをマージソートの中間状態と見なし、ソート済み区間の区切り位置を保持する
		// これが空の場合、intl_book_bodyは全てソート済みであると見なす
		IntlBookRun intl_book_run;

		// 読み込んだbookの名前
		std::string book_name;

		// メモリに丸読みせずにfind()のごとにファイルを調べにいくのか。
		bool on_the_fly = false;

		// 上のon_the_fly == trueのときに、開いている定跡ファイルのファイルハンドル
		std::ifstream fs;

		// 挿入ソートを行うしきい値
		const std::size_t MINRUN = 32;

		MemoryBook() : alloc(), intl_book_body(alloc) {}

		// intl_find()で見つからなかったときの値
		const it_ibe_t intl_end() { return intl_book_body.end(); }

		// find()で見つからなかったときの値（外部実装互換用）
		const extl_iter_t end() { return extl_book_body.end(); }

		// 比較関数
		struct less_inner_bebe {
			bool operator()(const ibe_t & lhs, const ibe_t & rhs) const { return bool(lhs.sfenPos < rhs.sfenPos); }
		};
		struct less_inner_strbe {
			bool operator()(const std::string & lhs, const ibe_t & rhs) const { return bool(lhs < rhs.sfenPos); }
		};
		struct less_inner_bestr {
			bool operator()(const ibe_t & lhs, const std::string & rhs) const { return bool(lhs.sfenPos < rhs); }
		};

		// intl_book_body内の定跡を探索
		it_ibe_t intl_find(const ibe_t & be)
		{
			auto it0 = intl_book_body.begin();
			auto itr = intl_book_run.begin();
			while (true)
			{
				auto it1 = (itr != intl_book_run.end()) ? std::next(intl_book_body.begin(), *itr) : intl_book_body.end();
				auto itf = std::lower_bound(it0, it1, be, less_inner_bebe());
				if (itf != it1 && *itf == be)
					return itf;
				if (itr == intl_book_run.end())
					return intl_end();
				itr++;
				it0 = it1;
			}
		}

		it_ibe_t intl_find(const std::string & sfenPos)
		{
			auto it0 = intl_book_body.begin();
			auto itr = intl_book_run.begin();
			while (true)
			{
				auto it1 = (itr != intl_book_run.end()) ? std::next(intl_book_body.begin(), *itr) : intl_book_body.end();
				auto itf = std::lower_bound(it0, it1, sfenPos, less_inner_bestr());
				if (itf != it1 && (*itf).sfenPos == sfenPos)
					return itf;
				if (itr == intl_book_run.end())
					return intl_end();
				itr++;
				it0 = it1;
			}
		}

		it_ibe_t intl_find(const Position & pos)
		{
			return intl_find(pos.trimedsfen());
		}

		// 定跡として登録されているかを調べて返す。
		// readのときにon_the_flyが指定されていればファイルを調べに行く。
		// (このとき、見つからなければthis::end()が返ってくる。
		// ファイルに読み込みに行っていることを意識する必要はない。)
		extl_iter_t find(const Position & pos);

		// BookRun再構築
		// 外部でbook_bodyに破壊的な操作を行った後に実行する
		void run_rebuild()
		{
			intl_book_run.clear();
			auto first = intl_book_body.begin(), mid = first, last = intl_book_body.end();
			while ((mid = std::is_sorted_until(mid, last)) != last)
				intl_book_run.push_back(std::distance(first, mid));
		}

		// マージソート(部分的)
		// length[n-1] > length[n] && length[n-2] > (length[n-1] + length[n]) の状態を満たすまでマージソートを行う。
		void intl_merge_part()
		{
			bool f = !intl_book_run.empty();
			auto end_d = std::distance(intl_book_body.begin(), intl_book_body.end());
			while (f)
			{
				f = false;
				auto run_size = intl_book_run.size();
				switch (run_size)
				{
				case 1:
					if (intl_book_run.front() <= end_d - intl_book_run.front())
					{
						std::inplace_merge(intl_book_body.begin(), intl_book_body.begin() + intl_book_run.front(), intl_book_body.end());
						intl_book_run.pop_back();
					}
					break;
				case 2:
					if (intl_book_run.back() - intl_book_run.front() <= end_d - intl_book_run.back())
					{
						std::inplace_merge(intl_book_body.begin() + intl_book_run.front(), intl_book_body.begin() + intl_book_run.back(), intl_book_body.end());
						intl_book_run.pop_back();
						f = true;
					} else if (intl_book_run.front() <= end_d - intl_book_run.front())
					{
						std::inplace_merge(intl_book_body.begin() + intl_book_run.front(), intl_book_body.begin() + intl_book_run.back(), intl_book_body.end());
						intl_book_run.pop_back();
						std::inplace_merge(intl_book_body.begin(), intl_book_body.begin() + intl_book_run.front(), intl_book_body.end());
						intl_book_run.pop_back();
					}
					break;
				default:
					if (
						intl_book_run.back() - intl_book_run[run_size - 2] <= end_d - intl_book_run.back() ||
						intl_book_run[run_size - 2] - intl_book_run[run_size - 3] <= end_d - intl_book_run[run_size - 2]
					)
					{
						std::inplace_merge(
							intl_book_body.begin() + intl_book_run[run_size - 2],
							intl_book_body.begin() + intl_book_run.back(),
							intl_book_body.end()
						);
						intl_book_run.pop_back();
						f = true;
					}
					break;
				}
			}
		}

		// マージソート(全体)
		void intl_merge()
		{
			while (!intl_book_run.empty())
			{
				auto d1 = intl_book_run.back();
				intl_book_run.pop_back();
				auto d0 = intl_book_run.empty() ? std::distance(intl_book_body.begin(), intl_book_body.begin()) : intl_book_run.back();
				std::inplace_merge(intl_book_body.begin() + d0, intl_book_body.begin() + d1, intl_book_body.end());
			}
		}

		// 同一局面の整理
		// 大量に局面登録する際、毎度重複チェックを行うと計算量が莫大になるのでまとめて処理
		void intl_uniq()
		{
			// 予め全体をソートしておく
			intl_merge();

			if (intl_book_body.empty()) return;
			std::size_t i = 1;
			it_ibe_t it0 = intl_book_body.begin(), it1 = it0 + 1, end = intl_book_body.end();
			while (it1 != end)
				if (*it0 == *it1)
				{
					do {
						(*it0).update(std::move(*it1++));
					} while (it1 != end && *it0 == *it1);
					it0 = intl_book_body.erase(++it0, it1);
					end = intl_book_body.end();
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
		void add(ibe_t & be, bool dofind = false)
		{
			it_ibe_t it;
			int cmp;
			if (dofind && (it = intl_find(be)) != intl_end())
			{
				// 重複していたら合成して再登録
				(*it).update(be);
			} else if (intl_book_body.empty() || (cmp = be.compare(intl_book_body.back())) > 0)
			{
				// 順序関係が保たれているなら単純に末尾に追加
				intl_book_body.push_back(be);
			} else if (cmp == 0)
			{
				// 末尾と同一局面なら合成
				intl_book_body.back().update(be);
			} else if (intl_book_run.empty())
			{
				// 既に全体がソート済みの場合、全体の長さを見て
				if (intl_book_body.size() < MINRUN)
				{
					// 短ければ、挿入ソート
					intl_book_body.insert(upper_bound(intl_book_body.begin(), intl_book_body.end(), be), be);
				} else
				{
					// 長ければ、新しいソート済み範囲を追加
					intl_book_run.push_back(std::distance(intl_book_body.begin(), intl_book_body.end()));
					intl_book_body.push_back(be);
				}
			} else
			{
				// ソート済み範囲が区分化されている場合、末尾の範囲の長さを見て
				if (std::distance(intl_book_body.begin() + intl_book_run.back(), intl_book_body.end()) < (intl_diff_t)MINRUN)
				{
					// 短ければ、挿入ソート
					intl_book_body.insert(std::upper_bound(intl_book_body.begin() + intl_book_run.back(), intl_book_body.end(), be), be);
				} else
				{
					// 長ければ、部分的なマージソートを試みてから
					intl_merge_part();
					// 新しいソート済み範囲を追加
					intl_book_run.push_back(std::distance(intl_book_body.begin(), intl_book_body.end()));
					intl_book_body.push_back(be);
				}
			}
		}

		// 局面・指し手の追加
		void insert_book_pos(const std::pair<const std::string, const int> & sfen, IntlBookPos & bp)
		{
			auto it = intl_find(sfen.first);
			if (it == intl_end())
			{
				// 存在しないので要素を作って追加。
				ibe_t be(sfen);
				be.move_list.push_front(bp);
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
			intl_book_body.clear();
			intl_book_run.clear();
			extl_book_body.clear();
		}

	};

	// 出力ストリーム
	std::ostream & operator << (std::ostream & os, const BookPos & bp);
	std::ostream & operator << (std::ostream & os, const BookEntry & be);
	std::ostream & operator << (std::ostream & os, const IntlBookPos & bp);
	std::ostream & operator << (std::ostream & os, const ibe_t & be);

	// 定跡ファイルの読み込み(book.db)など。
	// 同じファイルを二度目は読み込み動作をskipする。
	// on_the_flyが指定されているとメモリに丸読みしない。
	// 定跡作成時などはこれをtrueにしてはいけない。(メモリに読み込まれないため)
	extern int read_book(const std::string & filename, MemoryBook & book, bool on_the_fly = false, bool sfen_n11n = false);

	// 定跡ファイルの書き出し
	extern int write_book(const std::string & filename, MemoryBook & book);

#ifdef ENABLE_MAKEBOOK_CMD
	// USI拡張コマンド。"makebook"。定跡ファイルを作成する。
	// フォーマット等についてはdoc/解説.txt を見ること。
	extern void makebook_cmd(Position & pos, std::istringstream & is);
#endif


}

#endif // #ifndef _BOOK_H_
