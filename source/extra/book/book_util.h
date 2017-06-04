#ifndef _BOOK_UTIL_H_
#define _BOOK_UTIL_H_

#include "book.h"
#include "apery_book.h"
#include "../../thread.h"
#include "../kif_converter/kif_convert_tools.h"
#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
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
	std::string fetch_initialpos(Position & pos, std::istream & is, Thread* th = Threads.main());

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
	typedef std::shared_ptr<MoveListType> MoveListPtr;

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
		MoveListPtr move_list;

		BookEntry() : SfenPos(), move_list() {}
		BookEntry(const std::string sfen, const int p) : SfenPos(sfen, p), move_list() {}
		BookEntry(const sfen_pair_t & sfen_pair) : SfenPos(sfen_pair), move_list() {}
		BookEntry(const sfen_pair_t & sfen_pair, const MoveListPtr & mlist) : SfenPos(sfen_pair), move_list(mlist) {}
		BookEntry(const Position & pos) : SfenPos(pos), move_list() {}
		BookEntry(const Position & pos, const MoveListPtr & mlist) : SfenPos(pos), move_list(mlist) {}

		// char文字列からの BookEntry 生成
		void init(const char * sfen, const size_t length, const bool sfen_n11n, Thread* th);
		BookEntry(const char * sfen, const size_t length, const bool sfen_n11n = false, Thread* th = Threads.main()) : move_list(new MoveListType) { init(sfen, length, sfen_n11n, th); }

		// ストリームからの BookPos 順次読み込み
		void incpos(std::istream & is, char * _buffer, const size_t _buffersize);

		// ストリームからの BookEntry 読み込み
		void init(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n, Thread* th);
		BookEntry(std::istream & is, char * _buffer, const size_t _buffersize, const bool sfen_n11n = false, Thread* th = Threads.main()) : move_list(new MoveListType)
		{
			init(is, _buffer, _buffersize, sfen_n11n, th);
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
				!be.move_list->empty() && (move_list->empty() ||
					// depth が深い方
					move_list->front().depth < be.move_list->front().depth ||
					(	// depth が同じなら MultiPV の多い方、それでも同じなら後者
						move_list->front().depth == be.move_list->front().depth &&
						move_list->size() <= be.move_list->size()
					))
				)
				move_list = be.move_list;

		}

		void sort_pos()
		{
			std::stable_sort(move_list->begin(), move_list->end());
		}

		void insert_book_pos(const BookPos & bp)
		{
			for (auto & b : *move_list)
			{
				if (b == bp)
				{
					auto num = b.num + bp.num;
					b = bp;
					b.num = num;
					return;
				}
			}
			move_list->push_back(bp);
		}

		std::vector<Book::BookPos> export_move_list(const Position & pos)
		{
			std::vector<Book::BookPos> mlist;
			mlist.reserve(move_list->size());
			u64 num_sum = 0;
			for (auto & bp : *move_list)
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
	static void move_update(const Position & pos, MoveListPtr & mlist) {
		for (auto & bp : *mlist)
			bp.bestMove = pos.move16_to_move(bp.bestMove);
	}

	// Book抽象定義
	struct AbstractBook
	{
		std::string book_name;
		virtual int read_book(const std::string & ) { return 1; }
		virtual int write_book(const std::string & ) { return 1; }
		virtual int close_book() { return 1; }
		virtual MoveListPtr get_entries(const Position & ) { return MoveListPtr(); }
	};

	// 単に全合法手を返すだけのサンプル実装
	struct RandomBook : AbstractBook
	{
		int read_book(const std::string & ) { return 0; }
		int close_book() { return 0; }
		MoveListPtr get_entries(const Position & pos)
		{
			MoveListPtr mlp(new MoveListType);
			for (ExtMove m : MoveList<LEGAL_ALL>(pos))
				mlp->emplace_back(m.move, MOVE_NONE, 0, 0, 1);
			return mlp;
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
		MoveListPtr get_entries(const Position & pos);
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

		int read_book(const std::string & filename, bool sfen_n11n = false, Thread* th = Threads.main());
		int write_book(const std::string & filename);
		int close_book();
		MoveListPtr get_entries(const Position & pos);
		deqBook() {}
		deqBook(const std::string & filename)
		{
			read_book(filename);
		}
		deqBook(const std::string & filename, bool sfen_n11n, Thread* th)
		{
			read_book(filename, sfen_n11n, th);
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
		MoveListPtr get_entries(const Position & pos)
		{
			const auto& apmlist = apery_book.get_entries(pos);
			if (!apmlist.empty())
				return MoveListPtr();
			MoveListPtr mlp(new MoveListType);
			for (const auto& entry : apmlist)
			{
				Move mv = convert_move_from_apery(entry.fromToPro);
				// collision check
				if (!pos.legal(mv))
					return MoveListPtr();
				mlp->emplace_back(pos.move16_to_move(mv), MOVE_NONE, entry.score, 256, entry.count);
			}
			return mlp;
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

	// 棋譜出力書式
	enum KifExportFormat : int
	{
		KIFEXPORT_SFEN,
		KIFEXPORT_KIF1,
		KIFEXPORT_KIF2,
		KIFEXPORT_CSA1,
		KIFEXPORT_CSA,
		KIFEXPORT_NB
	};

	// 1手目・2手目全変化
	const std::string OPENING_PLY2ALL = "2g2f 3c3d,2g2f 8c8d,2g2f 7a7b,2g2f 4a3b,2g2f 7a6b,2g2f 5a4b,2g2f 3a4b,2g2f 1c1d,2g2f 3a3b,2g2f 9c9d,2g2f 5a5b,2g2f 4a4b,2g2f 6a5b,2g2f 6a7b,2g2f 6a6b,2g2f 5a6b,2g2f 5c5d,2g2f 8b5b,2g2f 7c7d,2g2f 8b9b,2g2f 8b7b,2g2f 8b4b,2g2f 8b6b,2g2f 6c6d,2g2f 4c4d,2g2f 1a1b,2g2f 9a9b,2g2f 4a5b,2g2f 8b3b,2g2f 2c2d,7g7f 3c3d,7g7f 4a3b,7g7f 7c7d,7g7f 7a7b,7g7f 8c8d,7g7f 7a6b,7g7f 5a4b,7g7f 5c5d,7g7f 8b5b,7g7f 8b7b,7g7f 3a4b,7g7f 8b3b,7g7f 1c1d,7g7f 8b4b,7g7f 9c9d,7g7f 5a5b,7g7f 4a4b,7g7f 6a5b,7g7f 3a3b,7g7f 6a7b,7g7f 6c6d,7g7f 8b6b,7g7f 8b9b,7g7f 4a5b,7g7f 6a6b,7g7f 5a6b,7g7f 2c2d,7g7f 4c4d,7g7f 9a9b,7g7f 1a1b,3i3h 4a3b,3i3h 8c8d,3i3h 3c3d,3i3h 7a7b,3i3h 7a6b,3i3h 1c1d,3i3h 5a5b,3i3h 5a4b,3i3h 3a4b,3i3h 4c4d,3i3h 5a6b,3i3h 7c7d,3i3h 8b3b,3i3h 9c9d,3i3h 8b4b,3i3h 6a6b,3i3h 4a4b,3i3h 5c5d,3i3h 6a7b,3i3h 3a3b,3i3h 6a5b,3i3h 8b5b,3i3h 6c6d,3i3h 8b7b,3i3h 4a5b,3i3h 8b6b,3i3h 8b9b,3i3h 2c2d,3i3h 9a9b,3i3h 1a1b,5i6h 4a3b,5i6h 5a4b,5i6h 7a7b,5i6h 8c8d,5i6h 1c1d,5i6h 5a5b,5i6h 3c3d,5i6h 3a3b,5i6h 7a6b,5i6h 3a4b,5i6h 6a7b,5i6h 5a6b,5i6h 9c9d,5i6h 4c4d,5i6h 7c7d,5i6h 5c5d,5i6h 6a5b,5i6h 6a6b,5i6h 4a4b,5i6h 8b3b,5i6h 8b5b,5i6h 8b4b,5i6h 6c6d,5i6h 8b6b,5i6h 8b9b,5i6h 4a5b,5i6h 8b7b,5i6h 2c2d,5i6h 9a9b,5i6h 1a1b,7i7h 3c3d,7i7h 8c8d,7i7h 7a7b,7i7h 7a6b,7i7h 4a3b,7i7h 7c7d,7i7h 5a4b,7i7h 3a4b,7i7h 5c5d,7i7h 4c4d,7i7h 6a5b,7i7h 3a3b,7i7h 5a5b,7i7h 9c9d,7i7h 6a6b,7i7h 8b5b,7i7h 8b4b,7i7h 8b3b,7i7h 1c1d,7i7h 6a7b,7i7h 4a4b,7i7h 5a6b,7i7h 6c6d,7i7h 4a5b,7i7h 8b6b,7i7h 8b9b,7i7h 8b7b,7i7h 2c2d,7i7h 9a9b,7i7h 1a1b,6i7h 4c4d,6i7h 3c3d,6i7h 8b4b,6i7h 4a3b,6i7h 8b5b,6i7h 8b3b,6i7h 7a7b,6i7h 8c8d,6i7h 3a4b,6i7h 5a4b,6i7h 5c5d,6i7h 8b7b,6i7h 9c9d,6i7h 7a6b,6i7h 4a4b,6i7h 4a5b,6i7h 7c7d,6i7h 8b9b,6i7h 8b6b,6i7h 1c1d,6i7h 3a3b,6i7h 6c6d,6i7h 6a5b,6i7h 5a6b,6i7h 6a7b,6i7h 6a6b,6i7h 5a5b,6i7h 2c2d,6i7h 1a1b,6i7h 9a9b,5g5f 3c3d,5g5f 8c8d,5g5f 5a4b,5g5f 7a6b,5g5f 7a7b,5g5f 6a5b,5g5f 6c6d,5g5f 4a3b,5g5f 7c7d,5g5f 6a6b,5g5f 5a5b,5g5f 6a7b,5g5f 5c5d,5g5f 9c9d,5g5f 3a4b,5g5f 3a3b,5g5f 1c1d,5g5f 4a4b,5g5f 4a5b,5g5f 4c4d,5g5f 5a6b,5g5f 8b3b,5g5f 8b5b,5g5f 8b7b,5g5f 8b6b,5g5f 8b4b,5g5f 8b9b,5g5f 9a9b,5g5f 1a1b,5g5f 2c2d,3i4h 8c8d,3i4h 7a7b,3i4h 3c3d,3i4h 4a3b,3i4h 5a5b,3i4h 5a4b,3i4h 7a6b,3i4h 1c1d,3i4h 3a4b,3i4h 6a7b,3i4h 6c6d,3i4h 4c4d,3i4h 7c7d,3i4h 9c9d,3i4h 5a6b,3i4h 6a6b,3i4h 5c5d,3i4h 6a5b,3i4h 8b3b,3i4h 4a4b,3i4h 3a3b,3i4h 8b4b,3i4h 8b5b,3i4h 4a5b,3i4h 8b6b,3i4h 8b9b,3i4h 8b7b,3i4h 1a1b,3i4h 9a9b,3i4h 2c2d,5i5h 4a3b,5i5h 3c3d,5i5h 7a7b,5i5h 8c8d,5i5h 5a5b,5i5h 5a4b,5i5h 7a6b,5i5h 9c9d,5i5h 3a4b,5i5h 3a3b,5i5h 4c4d,5i5h 5c5d,5i5h 8b3b,5i5h 8b5b,5i5h 7c7d,5i5h 1c1d,5i5h 4a4b,5i5h 8b4b,5i5h 6a5b,5i5h 6a7b,5i5h 4a5b,5i5h 5a6b,5i5h 6a6b,5i5h 6c6d,5i5h 8b9b,5i5h 8b6b,5i5h 8b7b,5i5h 9a9b,5i5h 2c2d,5i5h 1a1b,9g9f 8c8d,9g9f 3c3d,9g9f 4a3b,9g9f 7a7b,9g9f 3a3b,9g9f 5a5b,9g9f 5a4b,9g9f 7a6b,9g9f 1c1d,9g9f 4a4b,9g9f 7c7d,9g9f 9c9d,9g9f 5a6b,9g9f 6a5b,9g9f 5c5d,9g9f 4c4d,9g9f 3a4b,9g9f 6a7b,9g9f 8b5b,9g9f 6a6b,9g9f 8b4b,9g9f 8b3b,9g9f 4a5b,9g9f 8b6b,9g9f 6c6d,9g9f 8b7b,9g9f 8b9b,9g9f 2c2d,9g9f 9a9b,9g9f 1a1b,4i5h 5a4b,4i5h 7a7b,4i5h 3c3d,4i5h 8c8d,4i5h 4a3b,4i5h 7a6b,4i5h 3a3b,4i5h 9c9d,4i5h 1c1d,4i5h 7c7d,4i5h 5a5b,4i5h 4a4b,4i5h 6a5b,4i5h 3a4b,4i5h 6c6d,4i5h 6a6b,4i5h 5a6b,4i5h 6a7b,4i5h 4c4d,4i5h 5c5d,4i5h 8b3b,4i5h 8b5b,4i5h 8b4b,4i5h 4a5b,4i5h 8b9b,4i5h 8b7b,4i5h 8b6b,4i5h 2c2d,4i5h 1a1b,4i5h 9a9b,7i6h 3c3d,7i6h 7a7b,7i6h 7c7d,7i6h 5a4b,7i6h 7a6b,7i6h 4a3b,7i6h 8c8d,7i6h 3a4b,7i6h 1c1d,7i6h 4c4d,7i6h 5c5d,7i6h 6c6d,7i6h 9c9d,7i6h 6a7b,7i6h 3a3b,7i6h 6a5b,7i6h 8b5b,7i6h 4a4b,7i6h 6a6b,7i6h 5a5b,7i6h 4a5b,7i6h 8b3b,7i6h 5a6b,7i6h 8b4b,7i6h 8b7b,7i6h 8b9b,7i6h 8b6b,7i6h 2c2d,7i6h 9a9b,7i6h 1a1b,1g1f 7a7b,1g1f 8c8d,1g1f 3c3d,1g1f 5a4b,1g1f 4a3b,1g1f 9c9d,1g1f 3a3b,1g1f 1c1d,1g1f 7a6b,1g1f 4c4d,1g1f 5a5b,1g1f 5c5d,1g1f 6a7b,1g1f 8b5b,1g1f 4a4b,1g1f 3a4b,1g1f 8b3b,1g1f 6a5b,1g1f 7c7d,1g1f 8b4b,1g1f 5a6b,1g1f 6c6d,1g1f 6a6b,1g1f 8b7b,1g1f 4a5b,1g1f 8b6b,1g1f 8b9b,1g1f 2c2d,1g1f 1a1b,1g1f 9a9b,6g6f 3c3d,6g6f 3a3b,6g6f 8c8d,6g6f 7a6b,6g6f 6c6d,6g6f 7a7b,6g6f 5c5d,6g6f 3a4b,6g6f 5a4b,6g6f 4a4b,6g6f 6a5b,6g6f 4a3b,6g6f 7c7d,6g6f 9c9d,6g6f 6a7b,6g6f 6a6b,6g6f 4c4d,6g6f 8b5b,6g6f 5a5b,6g6f 8b3b,6g6f 1c1d,6g6f 8b6b,6g6f 4a5b,6g6f 8b7b,6g6f 8b4b,6g6f 5a6b,6g6f 8b9b,6g6f 2c2d,6g6f 1a1b,6g6f 9a9b,3g3f 8c8d,3g3f 3c3d,3g3f 7a7b,3g3f 5a5b,3g3f 5a4b,3g3f 7a6b,3g3f 4a3b,3g3f 3a3b,3g3f 1c1d,3g3f 4c4d,3g3f 8b3b,3g3f 3a4b,3g3f 5c5d,3g3f 6a6b,3g3f 6a7b,3g3f 9c9d,3g3f 4a4b,3g3f 6a5b,3g3f 7c7d,3g3f 5a6b,3g3f 8b5b,3g3f 4a5b,3g3f 8b4b,3g3f 6c6d,3g3f 2c2d,3g3f 8b9b,3g3f 8b6b,3g3f 8b7b,3g3f 9a9b,3g3f 1a1b,6i6h 8c8d,6i6h 3c3d,6i6h 7a7b,6i6h 4a3b,6i6h 5a4b,6i6h 5a5b,6i6h 3a4b,6i6h 7c7d,6i6h 7a6b,6i6h 1c1d,6i6h 9c9d,6i6h 4a4b,6i6h 6a7b,6i6h 6a5b,6i6h 4c4d,6i6h 3a3b,6i6h 8b5b,6i6h 8b4b,6i6h 5c5d,6i6h 6a6b,6i6h 5a6b,6i6h 6c6d,6i6h 8b3b,6i6h 8b7b,6i6h 8b6b,6i6h 8b9b,6i6h 4a5b,6i6h 2c2d,6i6h 9a9b,6i6h 1a1b,4i3h 7a7b,4i3h 4a3b,4i3h 7a6b,4i3h 8c8d,4i3h 3c3d,4i3h 5a5b,4i3h 5a4b,4i3h 9c9d,4i3h 1c1d,4i3h 3a4b,4i3h 5a6b,4i3h 3a3b,4i3h 4a4b,4i3h 6a7b,4i3h 4c4d,4i3h 7c7d,4i3h 6a6b,4i3h 6a5b,4i3h 8b3b,4i3h 6c6d,4i3h 8b5b,4i3h 5c5d,4i3h 8b4b,4i3h 4a5b,4i3h 8b9b,4i3h 8b6b,4i3h 8b7b,4i3h 9a9b,4i3h 2c2d,4i3h 1a1b,2h5h 8c8d,2h5h 3c3d,2h5h 7a6b,2h5h 7c7d,2h5h 5a4b,2h5h 3a3b,2h5h 1c1d,2h5h 6a7b,2h5h 6a5b,2h5h 6c6d,2h5h 7a7b,2h5h 3a4b,2h5h 6a6b,2h5h 4c4d,2h5h 8b3b,2h5h 5c5d,2h5h 8b4b,2h5h 5a5b,2h5h 4a3b,2h5h 4a5b,2h5h 9c9d,2h5h 8b5b,2h5h 4a4b,2h5h 8b6b,2h5h 8b7b,2h5h 2c2d,2h5h 8b9b,2h5h 5a6b,2h5h 1a1b,2h5h 9a9b,2h7h 5a4b,2h7h 7a6b,2h7h 8c8d,2h7h 1c1d,2h7h 3a4b,2h7h 5c5d,2h7h 6a5b,2h7h 7a7b,2h7h 7c7d,2h7h 9c9d,2h7h 6c6d,2h7h 6a7b,2h7h 5a5b,2h7h 6a6b,2h7h 3c3d,2h7h 3a3b,2h7h 4a4b,2h7h 4a5b,2h7h 8b3b,2h7h 4a3b,2h7h 2c2d,2h7h 8b9b,2h7h 4c4d,2h7h 8b5b,2h7h 5a6b,2h7h 8b6b,2h7h 8b7b,2h7h 8b4b,2h7h 1a1b,2h7h 9a9b,4i4h 8c8d,4i4h 4a3b,4i4h 7a6b,4i4h 3c3d,4i4h 5a4b,4i4h 7a7b,4i4h 5a5b,4i4h 1c1d,4i4h 5a6b,4i4h 4a4b,4i4h 9c9d,4i4h 7c7d,4i4h 3a4b,4i4h 3a3b,4i4h 6a6b,4i4h 5c5d,4i4h 6a5b,4i4h 6c6d,4i4h 4c4d,4i4h 8b5b,4i4h 8b3b,4i4h 8b4b,4i4h 6a7b,4i4h 4a5b,4i4h 8b7b,4i4h 8b9b,4i4h 9a9b,4i4h 8b6b,4i4h 1a1b,4i4h 2c2d,5i4h 3c3d,5i4h 4a3b,5i4h 4c4d,5i4h 5c5d,5i4h 9c9d,5i4h 8b5b,5i4h 8c8d,5i4h 4a4b,5i4h 7a7b,5i4h 7a6b,5i4h 1c1d,5i4h 5a4b,5i4h 3a4b,5i4h 5a5b,5i4h 8b3b,5i4h 8b4b,5i4h 7c7d,5i4h 6a5b,5i4h 3a3b,5i4h 5a6b,5i4h 6a6b,5i4h 8b7b,5i4h 6c6d,5i4h 4a5b,5i4h 6a7b,5i4h 8b9b,5i4h 8b6b,5i4h 1a1b,5i4h 2c2d,5i4h 9a9b,4g4f 8c8d,4g4f 7a6b,4g4f 4a3b,4g4f 7a7b,4g4f 1c1d,4g4f 5a4b,4g4f 3a3b,4g4f 7c7d,4g4f 5a5b,4g4f 3c3d,4g4f 9c9d,4g4f 6a7b,4g4f 3a4b,4g4f 6a6b,4g4f 6a5b,4g4f 8b4b,4g4f 6c6d,4g4f 4c4d,4g4f 5a6b,4g4f 4a4b,4g4f 8b3b,4g4f 5c5d,4g4f 8b5b,4g4f 8b7b,4g4f 8b9b,4g4f 8b6b,4g4f 4a5b,4g4f 2c2d,4g4f 1a1b,4g4f 9a9b,2h6h 8c8d,2h6h 5a4b,2h6h 7c7d,2h6h 7a6b,2h6h 3a4b,2h6h 6a5b,2h6h 1c1d,2h6h 3a3b,2h6h 5c5d,2h6h 4a4b,2h6h 6a7b,2h6h 3c3d,2h6h 7a7b,2h6h 5a5b,2h6h 9c9d,2h6h 6a6b,2h6h 8b3b,2h6h 4c4d,2h6h 4a5b,2h6h 6c6d,2h6h 4a3b,2h6h 8b7b,2h6h 8b5b,2h6h 8b4b,2h6h 8b6b,2h6h 5a6b,2h6h 8b9b,2h6h 2c2d,2h6h 1a1b,2h6h 9a9b,6i5h 3c3d,6i5h 7a7b,6i5h 7a6b,6i5h 8c8d,6i5h 5a4b,6i5h 4a4b,6i5h 6a5b,6i5h 6c6d,6i5h 9c9d,6i5h 1c1d,6i5h 5a5b,6i5h 6a7b,6i5h 7c7d,6i5h 3a4b,6i5h 4a3b,6i5h 6a6b,6i5h 5c5d,6i5h 3a3b,6i5h 8b5b,6i5h 4c4d,6i5h 5a6b,6i5h 4a5b,6i5h 8b4b,6i5h 8b3b,6i5h 8b7b,6i5h 8b9b,6i5h 1a1b,6i5h 8b6b,6i5h 9a9b,6i5h 2c2d,2h1h 3a3b,2h1h 8c8d,2h1h 7a7b,2h1h 7c7d,2h1h 5a4b,2h1h 3c3d,2h1h 7a6b,2h1h 1c1d,2h1h 6a6b,2h1h 4a4b,2h1h 6a5b,2h1h 3a4b,2h1h 6a7b,2h1h 5c5d,2h1h 4a3b,2h1h 5a5b,2h1h 9c9d,2h1h 6c6d,2h1h 4c4d,2h1h 8b5b,2h1h 8b4b,2h1h 8b3b,2h1h 5a6b,2h1h 4a5b,2h1h 8b9b,2h1h 8b7b,2h1h 2c2d,2h1h 8b6b,2h1h 1a1b,2h1h 9a9b,2h3h 8c8d,2h3h 3a3b,2h3h 7a6b,2h3h 7c7d,2h3h 1c1d,2h3h 7a7b,2h3h 5c5d,2h3h 5a4b,2h3h 3a4b,2h3h 6a5b,2h3h 6a6b,2h3h 9c9d,2h3h 5a5b,2h3h 3c3d,2h3h 6a7b,2h3h 6c6d,2h3h 4a4b,2h3h 4a3b,2h3h 8b3b,2h3h 8b5b,2h3h 8b4b,2h3h 4a5b,2h3h 5a6b,2h3h 4c4d,2h3h 8b7b,2h3h 2c2d,2h3h 8b9b,2h3h 8b6b,2h3h 1a1b,2h3h 9a9b,2h4h 3a3b,2h4h 8c8d,2h4h 7a7b,2h4h 5a4b,2h4h 7c7d,2h4h 7a6b,2h4h 3c3d,2h4h 4a4b,2h4h 1c1d,2h4h 6a5b,2h4h 5c5d,2h4h 3a4b,2h4h 5a5b,2h4h 6a7b,2h4h 9c9d,2h4h 6a6b,2h4h 6c6d,2h4h 4a3b,2h4h 8b4b,2h4h 8b5b,2h4h 8b3b,2h4h 5a6b,2h4h 4a5b,2h4h 4c4d,2h4h 8b9b,2h4h 2c2d,2h4h 8b7b,2h4h 8b6b,2h4h 1a1b,2h4h 9a9b,8g8f 8c8d,8g8f 5a4b,8g8f 3c3d,8g8f 7a7b,8g8f 3a3b,8g8f 6c6d,8g8f 9c9d,8g8f 6a6b,8g8f 7a6b,8g8f 5c5d,8g8f 7c7d,8g8f 5a5b,8g8f 1c1d,8g8f 4a3b,8g8f 3a4b,8g8f 6a5b,8g8f 6a7b,8g8f 4c4d,8g8f 4a4b,8g8f 4a5b,8g8f 8b5b,8g8f 5a6b,8g8f 8b3b,8g8f 8b4b,8g8f 8b6b,8g8f 8b7b,8g8f 8b9b,8g8f 1a1b,8g8f 9a9b,8g8f 2c2d,1i1h 3c3d,1i1h 9c9d,1i1h 7a6b,1i1h 1c1d,1i1h 5a4b,1i1h 8c8d,1i1h 7c7d,1i1h 3a3b,1i1h 6a5b,1i1h 5c5d,1i1h 4a4b,1i1h 4c4d,1i1h 7a7b,1i1h 3a4b,1i1h 4a5b,1i1h 6c6d,1i1h 6a7b,1i1h 4a3b,1i1h 8b4b,1i1h 8b5b,1i1h 5a5b,1i1h 6a6b,1i1h 8b3b,1i1h 8b7b,1i1h 2c2d,1i1h 8b9b,1i1h 8b6b,1i1h 1a1b,1i1h 5a6b,1i1h 9a9b,9i9h 7a7b,9i9h 8c8d,9i9h 5a4b,9i9h 3c3d,9i9h 7a6b,9i9h 1c1d,9i9h 4a4b,9i9h 4a3b,9i9h 9c9d,9i9h 3a4b,9i9h 3a3b,9i9h 7c7d,9i9h 5a5b,9i9h 6a6b,9i9h 6a5b,9i9h 6c6d,9i9h 6a7b,9i9h 5c5d,9i9h 5a6b,9i9h 4c4d,9i9h 8b4b,9i9h 8b7b,9i9h 8b6b,9i9h 4a5b,9i9h 8b3b,9i9h 8b5b,9i9h 8b9b,9i9h 2c2d,9i9h 1a1b,9i9h 9a9b";

#ifdef ENABLE_MAKEBOOK_CMD
	extern void bookutil_cmd(Position & pos, std::istringstream & is);
#endif

}

#endif // #ifndef _BOOK_UTIL_H_
