﻿#ifndef _BOOK_UTIL_H_
#define _BOOK_UTIL_H_

#include "book.h"
#include "apery_book.h"
#include "../kif_converter/kif_convert_tools.h"
#include "../stx/optional.h"
#include <algorithm>
#include <deque>
#include <iterator>
#include <utility>

namespace BookUtil
{

	typedef std::pair<std::string, int> sfen_pair_t;

	// 手数を除いた平手初期局面のsfen文字列
	const std::string SHORTSFEN_HIRATE = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -";

	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const std::string trim_sfen(const std::string sfen);
	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const std::string sfen);
	// sfen文字列の手数分離位置
	std::size_t trimlen_sfen(const char * sfen, const std::size_t length);
	std::size_t trimlen_sfen(const std::string sfen);
	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const std::string sfen);
	// 棋譜文字列ストリームから初期局面を抽出して設定
	std::string fetch_initialpos(Position & pos, std::istream & is);

	// 局面における指し手(定跡を格納するのに用いる)
	struct BookPos
	{

		Move bestMove;
		Move nextMove;
		int value;
		int depth;
		u64 num;

		BookPos(const BookPos & bp) : bestMove(bp.bestMove), nextMove(bp.nextMove), value(bp.value), depth(bp.depth), num(bp.num) {}
		BookPos(Move best, Move next, int v, int d, u64 n) : bestMove(best), nextMove(next), value(v), depth(d), num(n) {}

		// char文字列からのBookPos生成
		void init(const char * p);
		BookPos(const char * p) { init(p); }

		bool operator == (const BookPos & rhs) const
		{
			return bestMove == rhs.bestMove;
		}
		bool operator < (const BookPos & rhs) const
		{
			// std::sortで降順ソートされて欲しいのでこう定義する。
			return value > rhs.value || (value == rhs.value && num > rhs.num);
		}

	};

	typedef std::vector<BookPos> MoveListType;
	typedef MoveListType::iterator BookPosIter;
	typedef stx::optional<MoveListType> MoveListTypeOpt;

	// 内部実装用局面
	struct SfenPos
	{
		std::string sfenPos;
		int ply;

		SfenPos() : sfenPos(SHORTSFEN_HIRATE), ply(1) {}
		SfenPos(const std::string sfen, const int p) : sfenPos(sfen), ply(p) {}
		SfenPos(const sfen_pair_t & sfen_pair) : sfenPos(sfen_pair.first), ply(sfen_pair.second) {}
		SfenPos(const Position & pos) : sfenPos(pos.trimedsfen()), ply(pos.game_ply()) {}
		SfenPos(const SfenPos & pos) : sfenPos(pos.sfenPos), ply(pos.ply) {}

		// 局面比較
		bool operator < (const SfenPos & rhs) const
		{
			return sfenPos < rhs.sfenPos;
		}
		bool operator == (const SfenPos & rhs) const
		{
			return sfenPos == rhs.sfenPos;
		}
		int compare(const SfenPos & rhs) const
		{
			return sfenPos.compare(rhs.sfenPos);
		}
	};

	// 内部実装用局面項目
	struct BookEntry : SfenPos
	{
		MoveListType move_list;

		BookEntry() : SfenPos(), move_list() {}
		BookEntry(const std::string sfen, const int p) : SfenPos(sfen, p), move_list() {}
		BookEntry(const sfen_pair_t & sfen_pair) : SfenPos(sfen_pair), move_list() {}
		BookEntry(const sfen_pair_t & sfen_pair, const MoveListType & mlist) : SfenPos(sfen_pair), move_list(mlist) {}
		BookEntry(const Position & pos) : SfenPos(pos), move_list() {}
		BookEntry(const Position & pos, const MoveListType & mlist) : SfenPos(pos), move_list(mlist) {}

		// char文字列からの BookEntry 生成
		void init(const char * sfen, const size_t length, const bool sfen_n11n);
		BookEntry(const char * sfen, const size_t length, const bool sfen_n11n = false) { init(sfen, length, sfen_n11n); }

		// ストリームからの BookPos 順次読み込み
		void incpos(std::istream & is, char * _buffer, const size_t _buffersize);

		// ストリームからの BookEntry 読み込み
		void init(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n);
		BookEntry(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n = false)
		{
			init(is, _buffer, _buffersize, sfen_n11n);
		}

		// 局面比較
		bool operator < (const BookEntry & rhs) const
		{
			return sfenPos < rhs.sfenPos;
		}
		bool operator == (const BookEntry & rhs) const
		{
			return sfenPos == rhs.sfenPos;
		}
		int compare(const BookEntry & rhs) const
		{
			return sfenPos.compare(rhs.sfenPos);
		}

		// 優位な局面情報の選択
		void select(const BookEntry & be)
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
					(	// depth が同じなら MultiPV の多い方、それでも同じなら後者
						move_list.front().depth == be.move_list.front().depth &&
						move_list.size() <= be.move_list.size()
					))
				)
				move_list = be.move_list;

		}

		void sort_pos()
		{
			std::stable_sort(move_list.begin(), move_list.end());
		}

		void insert_book_pos(const BookPos & bp)
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

		std::vector<Book::BookPos> export_move_list(const Position & pos)
		{
			std::vector<Book::BookPos> mlist;
			mlist.reserve(move_list.size());
			u64 num_sum = 0;
			for (auto & bp : move_list)
			{
				// vector<BookUtil::BookPos> から vector<Book::BookPos> へのコピー
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
	static void move_update(const Position & pos, MoveListType & mlist) {
		for (auto & bp : mlist)
			bp.bestMove = pos.move16_to_move(bp.bestMove);
	}

	// Book抽象定義
	struct AbstractBook
	{
		std::string book_name;
		virtual int read_book(const std::string & ) { return 1; }
		virtual int write_book(const std::string & ) { return 1; }
		virtual int close_book() { return 1; }
		virtual MoveListTypeOpt get_entries(const Position & ) { return {}; }
	};

	// 単に全合法手を返すだけのサンプル実装
	struct RandomBook : AbstractBook
	{
		int read_book(const std::string & ) { return 0; }
		int close_book() { return 0; }
		MoveListTypeOpt get_entries(const Position & pos)
		{
			MoveListType mlist;
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
		MoveListTypeOpt get_entries(const Position & pos);
		OnTheFlyBook(const std::string & filename)
		{
			read_book(filename);
		}
	};

	typedef std::deque<BookEntry> BookType;
	typedef BookType::iterator BookIter;
	typedef std::iterator_traits<BookIter>::difference_type BookIterDiff;
	typedef std::vector<BookIterDiff> BookRun;

	// std::dequeを使った実装
	// std::unordered_map では順序関係が破壊されるが、 std::map は敢えて使わなかった的な。
	// 定跡同士のマージを行いやすくする（先頭要素のpopを多用する）ため、 std::vector ではなく std::deque を用いた。
	// 要素整列の戦略はTimSortの処理中の状態を参考に、必ずしも常にに全体を整列済みの状態には保たないものの、
	// ある程度要素追加中の二分探索も行いやすく（先に追加した要素ほど大きな整列済み区間に含まれる）、
	// 必要な際には低コストで全体を整列でき、元データがほぼ整列済みであればそれを利用する、
	// ただし実装簡易化のため逆順ソート済みだった区間をreverseする戦略は取らない
	//   { = 逆順ソート済み区間の整列は頻出しないと踏んで O(n) ではなく O(n log n) で妥協 }、
	// メモリ消費のオーバーヘッドはstd::mapより少なくする、などを目指す。
	struct deqBook : AbstractBook
	{

		// 定跡本体
		// dMemoryBook 同士のマージを行いやすくするため、
		BookType book_body;

		// ソート済み区間の保持
		// book_body をマージソートの中間状態とみなし、ソート済み区間の区切り位置を保持する
		// これが空の場合、 book_body は全てソート済みであるとみなす
		BookRun book_run;

		// 挿入ソートを行うしきい値
		const std::size_t MINRUN = 32;

		// find() で見つからなかった時の値
		const BookIter end();

		BookIter find(const BookEntry & be);
		BookIter find(const std::string & sfenPos);
		BookIter find(const Position & pos);

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
		int add(BookEntry & be, bool dofind = false);

		// 局面・指し手の追加
		void insert_book_pos(const sfen_pair_t & sfen, BookPos & bp);

		void clear();

		int read_book(const std::string & filename, bool sfen_n11n = false);
		int write_book(const std::string & filename);
		int close_book();
		MoveListTypeOpt get_entries(const Position & pos);
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
	struct AperyBook : AbstractBook
	{
		Book::AperyBook apery_book;
		int read_book(const std::string & filename)
		{
			apery_book = Book::AperyBook(filename.c_str());
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
		MoveListTypeOpt get_entries(const Position & pos)
		{
			auto apmlist = apery_book.get_entries_opt(pos);
			if (!apmlist)
				return {};
			MoveListType mlist;
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
		AperyBook(const std::string & filename) : apery_book(filename.c_str()) {}
	};

	// 比較関数
	struct less_bebe
	{
		bool operator()(const BookEntry & lhs, const BookEntry & rhs) const
		{
			return bool(lhs.sfenPos < rhs.sfenPos);
		}
	};
	struct less_bestr
	{
		bool operator()(const BookEntry & lhs, const std::string & rhs) const
		{
			return bool(lhs.sfenPos < rhs);
		}
	};

	// 数値 ←→ 文字列 変換
	void u32toa(char ** s, u32 i);
	void u32toa(char * s, u32 i);
	void s32toa(char ** s, s32 i);
	void s32toa(char * s, s32 i);
	void s64toa(char ** s, s64 i);
	void s64toa(char * s, s64 i);
	void u64toa(char ** s, u64 i);
	void u64toa(char * s, u64 i);
	u32 atou32(const char ** s);
	u32 atou32(const char * s);
	s32 atos32(const char ** s);
	s32 atos32(const char * s);
	u64 atou64(const char ** s);
	u64 atou64(const char * s);
	s64 atos64(const char ** s);
	s64 atos64(const char * s);

	// Move ←→ 文字列 変換
	void movetoa(char ** s, const Move m);
	void movetoa(char * s, const Move m);
	Move atomove(const char ** p);
	Move atomove(const char * p);

	// BookEntry → 文字列 変換
	void betoa(char ** s, const BookEntry & be);
	void betoa(char * s, const BookEntry & be);

	// 出力ストリーム
	std::ostream & operator << (std::ostream & os, const BookPos & bp);
	std::ostream & operator << (std::ostream & os, const BookEntry & be);

#ifdef ENABLE_MAKEBOOK_CMD
	extern void bookutil_cmd(Position & pos, std::istringstream & is);
#endif

}

#endif // #ifndef _BOOK_UTIL_H_
