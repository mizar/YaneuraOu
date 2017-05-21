#ifndef _BOOK_UTIL_H_
#define _BOOK_UTIL_H_

#include "book.h"
#include "apery_book.h"
#include "optional.h"
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
	typedef typename std::vector<dBookPos, alloc_dbp_t> dMoveListType;
	typedef typename dMoveListType::iterator dBookPosIter;
	typedef typename std::experimental::optional<dMoveListType> dMoveListTypeOpt;

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
		dMoveListType move_list;

		dBookEntry() : dSfenPos(), move_list() {}
		dBookEntry(const std::string sfen, const int p) : dSfenPos(sfen, p), move_list() {}
		dBookEntry(const sfen_pair_t & sfen_pair) : dSfenPos(sfen_pair), move_list() {}
		dBookEntry(const sfen_pair_t & sfen_pair, const dMoveListType & mlist) : dSfenPos(sfen_pair), move_list(mlist) {}
		dBookEntry(const Position & pos) : dSfenPos(pos), move_list() {}
		dBookEntry(const Position & pos, const dMoveListType & mlist) : dSfenPos(pos), move_list(mlist) {}

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

		std::vector<BookPos> export_move_list(const Position & pos)
		{
			std::vector<BookPos> mlist;
			mlist.reserve(move_list.size());
			u64 num_sum = 0;
			for (auto & bp : move_list)
			{
				// vector<dBookPos> から vector<BookPos> へのコピー
				// 定跡のMoveは16bitであり、rootMovesは32bitのMoveであるからこのタイミングで補正する。
				mlist.emplace_back(pos.move16_to_move(bp.bestMove), bp.nextMove, bp.value, bp.depth, bp.num);
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

	// 定跡のbestMove(16bit)を32bitに補正
	void move_update(const Position & pos, dMoveListType & mlist) {
		for (auto & bp : mlist)
			bp.bestMove = pos.move16_to_move(bp.bestMove);
	}

	// Book抽象定義
	struct AbstractBook
	{
		std::string book_name;
		virtual int read_book(const std::string & filename) { return 1; }
		virtual int write_book(const std::string & filename) { return 1; }
		virtual int close_book() { return 1; }
		virtual dMoveListTypeOpt get_entries(const Position & pos) { return {}; }
	};

	// 単に全合法手を返すだけのサンプル実装
	struct RandomBook : AbstractBook
	{
		int read_book(const std::string & filename) { return 0; }
		int close_book() { return 0; }
		dMoveListTypeOpt get_entries(const Position & pos)
		{
			dMoveListType mlist;
			for (ExtMove m : MoveList<LEGAL_ALL>(pos))
				mlist.emplace_back(m.move, MOVE_NONE, 0, 0, 1);
			return mlist;
		}
	};

	// OnTheFly読み込み用
	struct OnTheFlyBook : AbstractBook
	{
		// メモリに丸読みせずにfind()ごとにファイルを調べにいくのか。
		bool on_the_fly = false;
		// 上の on_the_fly == true の時に、開いている定跡ファイルのファイルハンドル
		std::ifstream fs;
		int read_book(const std::string & filename);
		int close_book();
		dMoveListTypeOpt get_entries(const Position & pos);
		OnTheFlyBook(const std::string & filename)
		{
			read_book(filename);
		}
	};

	typedef std::deque<dBookEntry> dBookType;
	typedef typename std::iterator_traits<dBookType::iterator>::difference_type dBookIterDiff;

	// std::dequeを使った実装
	// std::unordered_map では順序関係が破壊されるが、 std::map は敢えて使わなかった的な。
	// 定跡同士のマージを行いやすくする（先頭要素のpopを多用する）ため、 std::vector ではなく std::deque を用いた。
	// 要素整列の戦略はTimSortを参考に、必ずしも要素追加中に全体を厳格に整列済みの状態には保たないものの、
	// ある程度要素追加中の二分探索も行いやすく（先に追加した要素ほど大きな整列済み区間に含まれる）、必要な際には低コストで全体を整列でき、
	// 元データがほぼ整列済みであればそれを利用する（但し実装簡易化のため逆順ソート済みだった場合の整列処理効率 O(n) 化は行わず O(n log n) で妥協）、
	// メモリ消費のオーバーヘッドはstd::mapより少なくする、などを目指す。
	struct deqBook : AbstractBook
	{
		typedef std::vector<dBookIterDiff> dBookRun;

		// 定跡本体
		// dMemoryBook 同士のマージを行いやすくするため、
		dBookType book_body;

		// ソート済み区間の保持
		// book_body をマージソートの中間状態とみなし、ソート済み区間の区切り位置を保持する
		// これが空の場合、 book_body は全てソート済みであるとみなす
		dBookRun book_run;

		// 挿入ソートを行うしきい値
		const std::size_t MINRUN = 32;

		// find() で見つからなかった時の値
		const dBookType::iterator end()
		{
			return book_body.end();
		}

		// 比較関数
		struct d_less_bebe
		{
			bool operator()(const dBookEntry & lhs, const dBookEntry & rhs) const
			{
				return bool(lhs.sfenPos < rhs.sfenPos);
			}
		};
		struct d_less_bestr
		{
			bool operator()(const dBookEntry & lhs, const std::string & rhs) const
			{
				return bool(lhs.sfenPos < rhs);
			}
		};

		dBookType::iterator find(const dBookEntry & be)
		{
			auto it0 = book_body.begin();
			auto itr = book_run.begin();
			while (true)
			{
				auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
				auto itf = std::lower_bound(it0, it1, be, d_less_bebe());
				if (itf != it1 && *itf == be)
					return itf;
				if (itr == book_run.end())
					return end();
				++itr;
				it0 = it1;
			}
		}
		dBookType::iterator find(const std::string & sfenPos)
		{
			auto it0 = book_body.begin();
			auto itr = book_run.begin();
			while (true)
			{
				auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
				auto itf = std::lower_bound(it0, it1, sfenPos, d_less_bestr());
				if (itf != it1 && (*itf).sfenPos == sfenPos)
					return itf;
				if (itr == book_run.end())
					return end();
				++itr;
				it0 = it1;
			}
		}
		dBookType::iterator find(const Position & pos)
		{
			return find(pos.trimedsfen());
		}
		// 部分的マージソート（下記[1][2]が満たされるまで最後の2つの区間をマージソートする）
		// [1] ソート済み区間が2つ以上ある時、最後の区間は最後から2つ目の区間より小さい
		// [2] ソート済み区間が3つ以上ある時、(最後の区間+最後から2つ目の区間)は最後から3つ目の区間より小さい
		void intl_merge_part();

		// マージソート(全体)
		void intl_merge();

		// 同一局面の整理
		// 大量に局面登録する際、毎度重複チェックを行うよりまとめて処理する方が？
		void intl_uniq();

		// 局面の追加
		// 大量の局面を追加する場合、重複チェックを逐一は行わず(dofind_false)に、後で intl_uniq() を行うことを推奨
		int add(dBookEntry & be, bool dofind = false);

		// 局面・指し手の追加
		void insert_book_pos(const sfen_pair_t & sfen, dBookPos & bp)
		{
			auto it = find(sfen.first);
			if (it == end())
			{
				// 存在しないので要素を作って追加。
				dBookEntry be(sfen);
				be.move_list.push_back(bp);
				add(be);
			}
			else
				// この局面での指し手のリスト
				it->insert_book_pos(bp);
		}

		void clear()
		{
			book_body.clear();
			book_run.clear();
		}

		int read_book(const std::string & filename);
		int read_book(const std::string & filename, bool sfen_n11n);
		int write_book(const std::string & filename);
		int close_book();
		dMoveListTypeOpt get_entries(const Position & pos)
		{
			auto it = find(pos);
			if (it == end())
				return {};
			else
			{
				move_update(pos, it->move_list);
				return it->move_list;
			}
		}
		deqBook() {}
		deqBook(const std::string & filename)
		{
			read_book(filename);
		}
		deqBook(const std::string & filename, bool sfen_n11n)
		{
			read_book(filename, sfen_n11n);
		}
	};

	// AperyBook向けのインタフェース共通化用Adapter
	struct AdpAperyBook : AbstractBook
	{
		AperyBook apery_book;
		int read_book(const std::string & filename)
		{
			apery_book = AperyBook(filename.c_str());
			return 0;
		}
		static Move convert_move_from_apery(uint16_t apery_move) {
			const uint16_t to = apery_move & 0x7f;
			const uint16_t from = (apery_move >> 7) & 0x7f;
			const bool is_promotion = (apery_move & (1 << 14)) != 0;
			if (is_promotion) {
				return make_move_promote(static_cast<Square>(from), static_cast<Square>(to));
			}
			const bool is_drop = ((apery_move >> 7) & 0x7f) >= SQ_NB;
			if (is_drop) {
				const uint16_t piece = from - SQ_NB + 1;
				return make_move_drop(static_cast<Piece>(piece), static_cast<Square>(to));
			}
			return make_move(static_cast<Square>(from), static_cast<Square>(to));
		}
		dMoveListTypeOpt get_entries(const Position & pos)
		{
			auto apmlist = apery_book.get_entries_opt(pos);
			if (!apmlist)
				return {};
			dMoveListType mlist;
			for (const auto& entry : *apmlist)
			{
				Move mv = convert_move_from_apery(entry.fromToPro);
				// collision check
				if (!pos.legal(mv))
					return {};
				mlist.emplace_back(pos.move16_to_move(mv), MOVE_NONE, entry.score, 256, entry.count);
			}
			return mlist;
		}
		AdpAperyBook(const std::string & filename) : apery_book(filename.c_str()) {}
	};

	// 出力ストリーム
	std::ostream & operator << (std::ostream & os, const dBookPos & bp);
	std::ostream & operator << (std::ostream & os, const dBookEntry & be);

#ifdef ENABLE_MAKEBOOK_CMD
	extern void bookutil_cmd(Position & pos, std::istringstream & is);
#endif

}

#endif // #ifndef _BOOK_UTIL_H_
