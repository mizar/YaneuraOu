#ifndef _BOOK_H_
#define _BOOK_H_

#include <algorithm>
#include <deque>
#include <iterator>
#include <utility>
#include "../shogi.h"
#include "../position.h"

// 定跡処理関係
namespace Book
{
  // 局面における指し手(定跡を格納するのに用いる)
  struct BookPos
  {
    Move bestMove; // この局面での指し手
    Move nextMove; // その指し手を指したときの予想される相手の指し手
    int value;     // bestMoveを指したときの局面の評価値
    int depth;     // bestMoveの探索深さ
    uint64_t num;  // 何らかの棋譜集において、この指し手が採択された回数。
    float prob;    // ↑のnumをパーセンテージで表現したもの。(read_bookしたときには反映される。ファイルには書き出していない。)

    BookPos(Move best, Move next, int v, int d, uint64_t n) : bestMove(best), nextMove(next), value(v), depth(d), num(n) {}
    bool operator == (const BookPos& rhs) const { return bestMove == rhs.bestMove; }
    bool operator < (const BookPos& rhs) const { return num > rhs.num; } // std::sortで降順ソートされて欲しいのでこう定義する。
  };

  // sfen文字列の手数分離
	std::pair<string, int> split_sfen(string sfen)
	{
		string s = sfen;
		int cur = (int)s.length() - 1;
		while (cur >= 0)
		{
			char c = s[cur];
			// 改行文字、スペース、数字(これはgame ply)ではないならループを抜ける。
			// これらの文字が出現しなくなるまで末尾を切り詰める。
			if (c != '\r' && c != '\n' && c != ' ' && !('0' <= c && c <= '9'))
				break;
			cur--;
		}
		s.resize((int)(cur + 1));
		std::istringstream ss(sfen);
		int ply;
		ss.seekg((int)(cur + 1));
		ss >> ply;
		return std::make_pair(s, ply);
	}

  // 局面ごとの項目
  struct BookEntry
  {
    std::string sfenPos; // 末尾の手数を除くsfen局面文字列
    int ply; // 手数
    std::vector<BookPos> pvVec; // 候補手リスト

    // 初期化子
    BookEntry(std::string sfen, int p, std::vector<BookPos> pvec = {}) : sfenPos(sfen), ply(p), pvVec(pvec) {}
    BookEntry(Position pos, std::vector<BookPos> pvec = {}) {
      sfenPos = pos.trimedsfen();
      ply = pos.game_ply();
      pvVec = pvec;
    }
    BookEntry(std::string sfen, std::vector<BookPos> pvec = {}) {
      auto s = split_sfen(sfen);
      sfenPos = s.first;
      ply = s.second;
      pvVec = pvec;
    }
    // 局面比較
    bool operator < (const BookEntry& rhs) const { return sfenPos < rhs.sfePos; }
    bool operator == (const BookEntry& rhs) { return sfenPos == rhs.sfenPos; }
    // 同一局面の合成
    BookEntry operator + (const BookEntry& rhs) {
      BookEntry ret = { sfenPos, ply, pvVec };
      if (sfenPos == rhs.sfenPos) { // 同一局面のみ合成、異なる局面ならば前者
        if (rhs.ply > 0 && (ply > rhs.ply || ply < 1)) { ret.ply = rhs.ply; } // より早く出現する手数
        if (
          // 空では無い方
          !rhs.pvVec.empty() && (pvVec.empty() ||
          // depthが深い方
          pvVec[0].depth < rhs.pvVec[0].depth ||
          // depthが同じならMultiPVの多い方、それでも同じなら後者
          pvVec[0].depth == rhs.pvVec[0].depth && pvVec.size() <= rhs.pvVec.size()
        )) { ret.pvVec = rhs.pvVec; }
      }
      return ret;
    }
    // 候補手の追加
    void insert_book_pos (const BookPos& bp) {
      // 書きかけ
    }
    // 候補手の選択率算出
    void calc_prob () {
      std::stable_sort(pvVec.begin(), pvVec.end());
      uint64_t num_sum = 0;
      for (auto& bp : pvVec)
        num_sum += bp.num;
      for (auto& bp : pvVec)
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
    const BookType::iterator end() { return book_body.end(); }

    // book_body内の定跡を探索
    BookType::iterator intl_find(BookEntry& be) {
      auto it0 = book_body.begin();
      auto itr = book_run.begin();
      while (true) {
        auto it1 = (itr != book_run.end()) ? book_body.begin() + *itr : book_body.end();
        auto itf = std::lower_bound(it0, it1, be);
        if (*itf == be) { return itf; }
        if (itr == book_run.end()) { return end(); }
        itr++;
        it0 = it1;
      }
    }
    BookType::iterator intl_find(std::string sfen) {
      return intl_find(new BookEntry(sfen));
    }

    // 定跡として登録されているかを調べて返す。
    // readのときにon_the_flyが指定されていればファイルを調べに行く。
    // (このとき、見つからなければthis::end()が返ってくる。
    // ファイルに読み込みに行っていることを意識する必要はない。)
    BookType::iterator find(const Position& pos);

    // BookRun再構築
    // 外部でbook_bodyに破壊的な操作を行った後に実行する
    void run_rebuild() {
      book_run.clear();
      auto first = mid = book_body.begin(), last = book_body.end();
      while ((mid = std::is_sorted_until(mid, last)) != last) {
        book_run.push_back(std::distance(first, mid));
      }
    }

    // マージソート(部分的)
    // length[n-1] > length[n] && length[n-2] > (length[n-1] + length[n]) の状態を満たすまでマージソートを行う。
    void intl_merge_part() {
      bool f = !book_run.empty();
      auto end_d = std::distance(book_body.begin(), book_body.end());
      while (f) {
        f = false;
        switch (auto run_size = book_run.size()) {
          case 1:
            if (book_run.front() <= end_d - book_run.front()) {
              std::inplace_merge(book_body.begin(), book_body.begin() + book_run.front(), book_body.end());
              book_run.pop_back();
            }
            break;
          case 2:
            if (book_run.back() - book_run.front() <= end_d - book_run.back()) {
              std::inplace_merge(book_body.begin() + book_run.front(), book_body.begin() + book_run.back(), book_body.end());
              book_run.pop_back();
              f = true;
            } else if (book_run.front() <= end_d - book_run.front()) {
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
            ) {
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
    void intl_merge() {
      while (!book_run.empty()) {
        auto d1 = book_run.back();
        book_run.pop_back();
        auto d0 = book_run.empty() ? std::distance(book_body.begin(), book_body.begin()) : book_run.back();
        std::inplace_merge(book_body.begin() + d0, book_body.begin() + d1, book_body.end());
      }
    }

    // 同一局面の整理
    // 大量に局面登録する際、毎度重複チェックを行うと計算量が莫大になるのでまとめて処理
    void intl_uniq() {
      // 予め全体をソートしておく
      if (!book_run.empty()) { intl_merge(); }

      auto max = book_body.size();
      std::size_t i = 1;
      while (i < max) {
        if (book_body[i - 1] == book_body[i]) {
          book_body[i - 1] = book_body[i - 1] + book_body[i];
          book_body.erase(book_body.begin() + i);
          max = book_body.size();
        } else {
          ++i;
        }
      }
    }

    // 局面の追加
    // 大量の局面を追加する場合、重複チェックは逐一行わず(dofind=false)に、後でintl_uniq()を行う事を推奨
    void add(BookEntry& be, bool dofind = false) {
      if (dofind && (BookType::iterator it = intl_find(be)) != end()) {
        // 重複していたら合成して再登録
        *it = *it + be;
      } else if (book_body.empty() || !(be < book_body.back())) {
        // 順序関係が保たれているなら単純に末尾に追加
        book_body.push_back(be);
      } else if (book_run.empty()) {
        // 既に全体がソート済みの場合、全体の長さを見て
        if (book_body.size() < MINRUN) {
          // 短ければ、挿入ソート
          book_body.insert(upper_bound(book_body.begin(), book_body.end(), be), be);
        } else {
          // 長ければ、新しいソート済み範囲を追加
          book_run.push_back(std::distance(book_body.begin(), book_body.end()));
          book_body.push_back(be);
        }
      } else {
        // ソート済み範囲が区分化されている場合、末尾の範囲の長さを見て
        if (std::distance(book_body.begin() + book_run.back(), book_body.end()) < (diff_t)MINRUN) {
          // 短ければ、挿入ソート
          book_body.insert(std::upper_bound(book_body.begin() + book_run.back(), book_body.end(), be), be);
        } else {
          // 長ければ、部分的なマージソートを試みてから
          intl_merge_part();
          // 新しいソート済み範囲を追加
          book_run.push_back(std::distance(book_body.begin(), book_body.end()));
          book_body.push_back(be);
        }
      }
    }

    // 定跡のクリア
    void clear() { book_body.clear(); book_run.clear(); }
  };

  // 定跡ファイルの読み込み(book.db)など。
  // 同じファイルを二度目は読み込み動作をskipする。
  // on_the_flyが指定されているとメモリに丸読みしない。
  // 定跡作成時などはこれをtrueにしてはいけない。(メモリに読み込まれないため)
  extern int read_book(const std::string& filename, MemoryBook& book, bool on_the_fly = false);

  // 定跡ファイルの書き出し
  // sort = 書き出すときにsfen文字列で並び替えるのか。(書き出しにかかる時間増)
  extern int write_book(const std::string& filename, const MemoryBook& book, bool sort = false);

  // bookにBookPosを一つ追加。(その局面ですでに同じbestMoveの指し手が登録されている場合は上書き動作)
  extern void insert_book_pos(MemoryBook& book, const std::string sfen, const BookPos& bp);

  // book読み込み完了時の動作。(全体のソート・重複局面の整理など)
  extern void finalize_book(MemoryBook& book);

#ifdef ENABLE_MAKEBOOK_CMD
  // USI拡張コマンド。"makebook"。定跡ファイルを作成する。
  // フォーマット等についてはdoc/解説.txt を見ること。
  extern void makebook_cmd(Position& pos, std::istringstream& is);
#endif


}

#endif // #ifndef _BOOK_H_
