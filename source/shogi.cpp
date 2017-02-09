#include <sstream>
#include <iostream>

#include "shogi.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"

// ----------------------------------------
//    const
// ----------------------------------------

const char* USI_PIECE = ". P L N S B R G K +P+L+N+S+B+R+G+.p l n s b r g k +p+l+n+s+b+r+g+k";

// ----------------------------------------
//    tables
// ----------------------------------------

File SquareToFile[SQ_NB] = {
  FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1,
  FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2,
  FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3,
  FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4,
  FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5,
  FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6,
  FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7,
  FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8,
  FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9
};

Rank SquareToRank[SQ_NB] = {
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
};

std::string PieceToCharBW(" PLNSBRGK        plnsbrgk");


// ----------------------------------------
// operator<<(std::ostream& os,...)とpretty() 
// ----------------------------------------

std::string pretty(File f) { return pretty_jp ? std::string("１２３４５６７８９").substr((int32_t)f * 2, 2) : std::to_string((int32_t)f + 1); }
std::string pretty(Rank r) { return pretty_jp ? std::string("一二三四五六七八九").substr((int32_t)r * 2, 2) : std::to_string((int32_t)r + 1); }

std::string kif(File f, SquareFormat fmt)
{
	switch (fmt)
	{
	case SqFmt_FullWidthArabic:
	case SqFmt_FullWidthMix:
		return std::string("１２３４５６７８９").substr((int32_t)f * 2, 2);
	default:
		return std::to_string((int32_t)f + 1);
	}
}

std::string kif(Rank r, SquareFormat fmt)
{
	switch (fmt)
	{
	case SqFmt_FullWidthArabic:
		return std::string("１２３４５６７８９").substr((int32_t)r * 2, 2);
	case SqFmt_FullWidthMix:
		return std::string("一二三四五六七八九").substr((int32_t)r * 2, 2);
	default:
		return std::to_string((int32_t)r + 1);
	}
}

std::string pretty(Move m)
{
  if (is_drop(m))
    return (pretty(move_to(m)) + pretty2(Piece(move_from(m))) + (pretty_jp ? "打" : "*"));
  else
    return pretty(move_from(m)) + pretty(move_to(m)) + (is_promote(m) ? (pretty_jp ? "成" : "+") : "");
}

std::string pretty(Move m, Piece movedPieceType)
{
  if (is_drop(m))
    return (pretty(move_to(m)) + pretty2(movedPieceType) + (pretty_jp ? "打" : "*"));
  else
    return pretty(move_to(m)) + pretty2(movedPieceType) + (is_promote(m) ? (pretty_jp ? "成" : "+") : "") + "["+ pretty(move_from(m))+"]";
}

std::ostream& operator<<(std::ostream& os, Color c) { os << ((c == BLACK) ? (pretty_jp ? "先手" : "BLACK") : (pretty_jp ? "後手" : "WHITE")); return os; }

std::ostream& operator<<(std::ostream& os, Piece pc)
{
  auto s = usi_piece(pc);
  if (s[1] == ' ') s.resize(1); // 手動trim
  os << s;
  return os;
}

std::ostream& operator<<(std::ostream& os, Hand hand)
{
  for (Piece pr = PAWN; pr < PIECE_HAND_NB; ++pr)
  {
    int c = hand_count(hand, pr);
    // 0枚ではないなら出力。
    if (c != 0)
    {
      // 1枚なら枚数は出力しない。2枚以上なら枚数を最初に出力
      // PRETTY_JPが指定されているときは、枚数は後ろに表示。
      const std::string cs = (c != 1) ? std::to_string(c) : "";
      std::cout << (pretty_jp ? "" : cs) << pretty(pr) << (pretty_jp ? cs : "");
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, HandKind hk)
{
  for (Piece pc = PAWN; pc < PIECE_HAND_NB; ++pc)
    if (hand_exists(hk, pc))
      std::cout << pretty(pc);
  return os;
}

std::string to_usi_string(Move m)
{
  std::ostringstream ss;
  if (!is_ok(m))
  {
    ss <<((m == MOVE_RESIGN) ? "resign" :
          (m == MOVE_WIN   ) ? "win"    :
          (m == MOVE_NULL  ) ? "null"   :
          (m == MOVE_NONE  ) ? "none"   :
          "");
  }
  else if (is_drop(m))
  {
    ss << move_dropped_piece(m);
    ss << '*';
    ss << move_to(m);
  }
  else {
    ss << move_from(m);
    ss << move_to(m);
    if (is_promote(m))
      ss << '+';
  }
  return ss.str();
}
std::string to_kif1_string(Move m, Piece movedPieceType, Color c, Move prev_m, SquareFormat fmt)
{
	std::ostringstream ss;
	if (~c) ss << "▲"; else ss << "△";
	if (!is_ok(m))
	{
		switch (m) {
		case MOVE_NONE: ss << "エラー"; break;
		case MOVE_NULL: ss << "パス"; break;
		case MOVE_RESIGN: ss << "投了"; break;
		case MOVE_WIN: ss << "宣言勝ち"; break;
		}
	}
	else
	{
		if (is_ok(prev_m) && move_to(prev_m) == move_to(m))
			ss << "同" << kif(movedPieceType);
		else
			ss << kif(move_to(m), fmt) << kif(movedPieceType);
		if (is_promote(m))
			ss << "成";
		if (is_drop(m))
			ss << "打";
		else
			ss << "("
			<< std::to_string((int32_t)(file_of(move_from(m))) + 1)
			<< std::to_string((int32_t)(rank_of(move_from(m))) + 1)
			<< ")";
	}
	return ss.str();
}
std::string to_kif1_string(Move m, Position& pos, Move prev_m, SquareFormat fmt)
{
	return to_kif1_string(m, pos.moved_piece_before(m), pos.side_to_move(), prev_m, fmt);
}
std::string to_kif2_string(Move m, Position& pos, Move prev_m, SquareFormat fmt)
{
	std::ostringstream ss;
	Color c = pos.side_to_move();
	ss << (~c ? "▲" : "△");
	if (!is_ok(m))
	{
		switch (m) {
		case MOVE_NONE: ss << "エラー"; break;
		case MOVE_NULL: ss << "パス"; break;
		case MOVE_RESIGN: ss << "投了"; break;
		case MOVE_WIN: ss << "宣言勝ち"; break;
		}
	}
	else
	{
		// 先後・成の属性を含めた駒種別
		Piece p = pos.moved_piece_before(m);
		// 先後の属性を排除した駒種別
		Piece p_type = type_of(p);
		// 金・銀・成金は直の表記が有り得る
		bool is_like_goldsilver = (
			p_type == SILVER || 
			p_type == GOLD || 
			p_type == PRO_PAWN || 
			p_type == PRO_LANCE || 
			p_type == PRO_KNIGHT || 
			p_type == PRO_SILVER);
		// 移動元・移動先の座標
		Square fromSq = move_from(m), toSq = move_to(m);
		File fromSqF = file_of(fromSq), toSqF = file_of(toSq);
		Rank fromSqR = rank_of(fromSq), toSqR = rank_of(toSq);
		// 移動先地点への同駒種の利き（駒打ちで無ければ指し手の駒も含む）
		// Position.piecesの第2引数には先後の属性を含めてはいけない。
		Bitboard b = (pos.attackers_to(c, toSq) & pos.pieces(c, p_type));
		// 同駒種の利きの数
		int b_pop = b.pop_count();

		if (is_ok(prev_m) && move_to(prev_m) == toSq)
			ss << "同" << kif(p);
		else
			ss << kif(toSq, fmt) << kif(p);

		if (!is_drop(m)) {
			if (b_pop > 1) {
				if (fromSqR == toSqR) {
					if ((b & RANK_BB[toSqR]).pop_count() == 1)
						ss << "寄";
					else if (fromSqF < toSqF) {
						if ((b & InLeftBB[0][toSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else {
							if (~c) ss << "左寄"; else ss << "右寄";
						}
					}
					else if (fromSqF > toSqF) {
						if ((b & InRightBB[0][toSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else {
							if (~c) ss << "左寄"; else ss << "右寄";
						}
					}
				}
				else if (fromSqR < toSqR) {
					if ((b & InFrontBB[0][toSqR]).pop_count() == 1) {
						if (~c) ss << "引"; else ss << "上";
					}
					else if (fromSqF == toSqF) {
						if (c && is_like_goldsilver)
							ss << "直";
						else if (b_pop - (b & InLeftBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "右"; else ss << "左";
						}
						else if (b_pop - (b & InRightBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else if ((b & InLeftBB[0][toSqF] & InFrontBB[0][fromSqR]) == ZERO_BB) {
							if (~c) ss << "左引"; else ss << "右上";
						}
						else if ((b & InRightBB[0][toSqF] & InFrontBB[0][fromSqR]) == ZERO_BB) {
							if (~c) ss << "右引"; else ss << "左上";
						}
					}
					else if (fromSqF < toSqF) {
						if ((b & OrRightBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "右"; else ss << "左";
						}
						else {
							if (~c) ss << "右引"; else ss << "左上";
						}
					}
					else if (fromSqF > toSqF) {
						if ((b & OrLeftBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else {
							if (~c) ss << "左引"; else ss << "右上";
						}
					}
				}
				else if (fromSqR > toSqR) {
					if ((b & InBackBB[0][toSqR]).pop_count() == 1) {
						if (~c) ss << "上"; else ss << "引";
					}
					else if (fromSqF == toSqF) {
						if (~c && is_like_goldsilver)
							ss << "直";
						else if (b_pop - (b & InLeftBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "右"; else ss << "左";
						}
						else if (b_pop - (b & InRightBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else if ((b & InLeftBB[0][fromSqF] & InBackBB[0][toSqR]) == ZERO_BB) {
							if (~c) ss << "左上"; else ss << "右引";
						}
						else if ((b & InRightBB[0][fromSqF] & InBackBB[0][toSqR]) == ZERO_BB) {
							if (~c) ss << "右上"; else ss << "左引";
						}
					}
					else if (fromSqF < toSqF) {
						if ((b & OrRightBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "右"; else ss << "左";
						}
						else {
							if (~c) ss << "右上"; else ss << "左引";
						}
					}
					else if (fromSqF > toSqF) {
						if ((b & OrLeftBB[0][fromSqF]).pop_count() == 1) {
							if (~c) ss << "左"; else ss << "右";
						}
						else {
							if (~c) ss << "左上"; else ss << "右引";
						}
					}
				}
			}
			if (is_promote(m))
				ss << "成";
			else if (canPromote(c, fromSq, toSq))
				ss << "不成";
		}
		else if (b_pop > 0)
			ss << "打";
	}
	return ss.str();
}
std::string to_csa1_string(Move m, Piece movedPieceAfterType)
{
	if (!is_ok(m))
		return "";
	std::stringstream ss;
	if (is_drop(m))
		ss << "00";
	else
		ss
		<< std::to_string((int32_t)(file_of(move_from(m))) + 1)
		<< std::to_string((int32_t)(rank_of(move_from(m))) + 1);
	ss
		<< std::to_string((int32_t)(file_of(move_to(m))) + 1)
		<< std::to_string((int32_t)(rank_of(move_to(m))) + 1)
		<< csa(movedPieceAfterType);
	return ss.str();
}
std::string to_csa1_string(Move m, Position& pos)
{
	return to_csa1_string(m, pos.moved_piece_after(m));
}
std::string to_csa_string(Move m, Piece movedPieceAfterType, Color c)
{
	switch (m)
	{
	case MOVE_NONE:
	case MOVE_NULL:
		return "%ERROR";
	case MOVE_RESIGN:
		return "%TORYO";
	case MOVE_WIN:
		return "%WIN";
	}
	std::stringstream ss;
	ss << (~c ? "+" : "-");
	if (is_drop(m))
		ss << "00";
	else
		ss
		<< std::to_string((int32_t)(file_of(move_from(m))) + 1)
		<< std::to_string((int32_t)(rank_of(move_from(m))) + 1);
	ss
		<< std::to_string((int32_t)(file_of(move_to(m))) + 1)
		<< std::to_string((int32_t)(rank_of(move_to(m))) + 1)
		<< csa(movedPieceAfterType);
	return ss.str();
}
std::string to_csa_string(Move m, Position& pos)
{
	return to_csa_string(m, pos.moved_piece_after(m), pos.side_to_move());
}

// ----------------------------------------
// 探索用のglobalな変数
// ----------------------------------------

namespace Search {
  SignalsType Signals;
  LimitsType Limits;
  StateStackPtr SetupStates;

  // 探索を抜ける前にponderの指し手がないとき(rootでfail highしているだとか)にこの関数を呼び出す。
  // ponderの指し手として何かを指定したほうが、その分、相手の手番において考えられて得なので。

  bool RootMove::extract_ponder_from_tt(Position& pos,Move ponder_candidate)
  {
    StateInfo st;
    bool ttHit;

//    ASSERT_LV3(pv.size() == 1);

    // 詰みの局面が"ponderhit"で返ってくることがあるので、ここでのpv[0] == MOVE_RESIGNであることがありうる。
    if (!is_ok(pv[0]))
      return false;

    pos.do_move(pv[0], st, pos.gives_check(pv[0]));
    TTEntry* tte = TT.probe(pos.state()->key(), ttHit);
    Move m;
    if (ttHit)
    {
      m = tte->move(); // SMP safeにするためlocal copy
      if (MoveList<LEGAL_ALL>(pos).contains(m))
        goto FOUND;
    }
    // 置換表にもなかったので以前のiteration時のpv[1]をほじくり返す。
    m = ponder_candidate;
    if (MoveList<LEGAL_ALL>(pos).contains(m))
      goto FOUND;

    pos.undo_move(pv[0]);
    return false;
  FOUND:;
    pos.undo_move(pv[0]);
    pv.push_back(m);
//    std::cout << m << std::endl;
    return true;
  }

}

// 引き分け時のスコア(とそのdefault値)
Value drawValueTable[REPETITION_NB][COLOR_NB] =
{
  {  VALUE_ZERO        ,  VALUE_ZERO        }, // REPETITION_NONE
  {  VALUE_MATE        ,  VALUE_MATE        }, // REPETITION_WIN
  { -VALUE_MATE        , -VALUE_MATE        }, // REPETITION_LOSE
  {  VALUE_ZERO        ,  VALUE_ZERO        }, // REPETITION_DRAW  : このスコアはUSIのoptionコマンドで変更可能
  {  VALUE_SUPERIOR    ,  VALUE_SUPERIOR    }, // REPETITION_SUPERIOR
  { -VALUE_SUPERIOR    , -VALUE_SUPERIOR    }, // REPETITION_INFERIOR
};

// ----------------------------------------
//  main()
// ----------------------------------------

int main(int argc, char* argv[])
{
  // --- 全体的な初期化
  USI::init(Options);
  Bitboards::init();
  Position::init();
  Search::init();
  Threads.init();
  Eval::init(); // 簡単な初期化のみで評価関数の読み込みはisreadyに応じて行なう。

  // USIコマンドの応答部
  USI::loop(argc,argv);

  // 生成して、待機させていたスレッドの停止
  Threads.exit();

  return 0;
}
