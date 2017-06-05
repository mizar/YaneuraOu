#include "../shogi.h"
#include "../position.h"
#include "../evaluate.h"
#include "../misc.h"

// 全評価関数に共通の処理などもここに記述する。

namespace Eval
{
#if !defined (EVAL_NO_USE)
  // 何らかの評価関数を用いる以上、駒割りの計算は必須。
  // すなわち、EVAL_NO_USE以外のときはこの関数が必要。

  // 駒割りの計算
  // 手番側から見た評価値
	Value material(const Position& pos)
	{
		int v = VALUE_ZERO;

		for (auto i : SQ)
			v = v + PieceValue[pos.piece_on(i)];

		// 手駒も足しておく
		for (auto c : COLOR)
			for (auto pc = PAWN; pc < PIECE_HAND_NB; ++pc)
				v += (c == BLACK ? 1 : -1) * Value(hand_count(pos.hand_of(c), pc) * PieceValue[pc]);

		return (Value)v;
	}
#endif

#if defined (EVAL_MATERIAL)
	// 駒得のみの評価関数のとき。
	void init() {}
	void load_eval() {}
	void print_eval_stat(Position& pos) {}
	Value evaluate(const Position& pos) {
		auto score = pos.state()->materialValue;
		ASSERT_LV5(pos.state()->materialValue == Eval::material(pos));
		return pos.side_to_move() == BLACK ? score : -score;
	}
	Value compute_eval(const Position& pos) { return material(pos); }
#endif

#if defined(EVAL_KKPT) || defined(EVAL_KPPT)
	// calc_check_sum()を呼び出して返ってきた値を引数に渡すと、ソフト名を表示してくれる。
	void print_softname(u64 check_sum)
	{
		// 評価関数ファイルの正体
		std::string softname = "unknown";

		// ソフト名自動判別
		std::map<u64, std::string> list = {
			{ 0x7171a5469027ebf , "ShinYane(20161010)" } ,
			{ 0x71fc7fd40c668cc , "Ukamuse(sdt4)" } ,

			{ 0x65cd7c55a9d4cd9 , "elmo(WCSC27)" } ,
			{ 0x3aa68b055a020a8 , "Yomita(WCSC27)" } ,
			{ 0x702fb2ee5672156 , "Qhapaq(WCSC27)" } ,
			{ 0x6c54a1bcb654e37 , "tanuki(WCSC27)" } ,
			{ 0x000000000000000 , "ReZero-epoch0(201706)" } ,
			{ 0x4625f541c89c16e , "ReZero-epoch0.1(201706)" } ,
			{ 0x4a8aa0a7ef9f507 , "ReZero-epoch1(201706)" } ,
			{ 0x4cf0cd7f21c169a , "ReZero-epoch2(201706)" } ,
			{ 0x4deaf5702a6ee6e , "ReZero-epoch3(201706)" } ,
			{ 0x4e7562db3366cf2 , "ReZero-epoch4(201706)" } ,
			{ 0x4edbd5bc0c0ab12 , "ReZero-epoch5(201706)" } ,
			{ 0x4f36ba83ed99e91 , "ReZero-epoch6(201706)" } ,
			{ 0x57e8e00bde3a55f , "ReZero-epoch7(201706)" } ,
			{ 0x58e206e66c06a58 , "ReZero-epoch8(201706)" } ,
			{ 0x64dccf2f61f17f8 , "EloQhappa(20170511v1.0)" } ,
			{ 0x64a4b49026085c0 , "EloQhappa(20170521v1.1)" } ,
	};
		if (list.count(check_sum))
			softname = list[check_sum];

		sync_cout << "info string Eval Check Sum = " << std::hex << check_sum << std::dec
			<< " , Eval File = " << softname << sync_endl;
	}
#endif

#if defined (USE_EVAL_MAKE_LIST_FUNCTION)

	// compute_eval()やLearner::add_grad()からBonaPiece番号の組み換えのために呼び出される関数
	std::function<void(const Position&, BonaPiece[40], BonaPiece[40])> make_list_function;

	// 旧評価関数から新評価関数に変換するときにKPPのP(BonaPiece)がどう写像されるのかを定義したmapper。
	// EvalIO::eval_convert()の引数として渡される。
	std::vector<u16 /*BonaPiece*/> eval_mapper;
#endif

}
