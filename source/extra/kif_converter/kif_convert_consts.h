#if defined(USE_KIF_CONVERT_TOOLS)

#include <tuple>
#include "../../shogi.h"

// kif_convert_tools.cppで用いる文字列定数。
// SJIS(build時のlocale依存)、UTF-8/16/32それぞれの文字列を用意してある。

namespace KifConvertTools
{

#define SC(I, S) (std::get<I>(std::make_tuple(std::string(S), std::string(u8"" S), std::u16string(u"" S), std::u32string(U"" S), std::wstring(L"" S))))
#if defined _LINUX
#define SCU(I, S, R) (std::get<I>(std::make_tuple(std::string(S), std::string(u8"" S), std::u16string(u"" S), std::u32string(U"" S), std::wstring(L"" S))))
#else
#define SCU(I, S, R) (std::get<I>(std::make_tuple(std::string(R), std::string(u8"" S), std::u16string(u"" S), std::u32string(U"" S), std::wstring(L"" S))))
#endif

	// 文字定数群
	template <typename T> struct KifCharBase {};
	template <std::size_t I, typename T> struct KifConst : KifCharBase<T>
	{
		typedef T char_type;
		typedef std::basic_string<T> string_type;
		const string_type lf = SC(I,"\n");
		const string_type csa_color_black = SC(I,"+");
		const string_type csa_color_white = SC(I,"-");
		const string_type csa_move_none = SC(I,"%%ERROR");
		const string_type csa_move_null = SC(I,"%%PASS");
		const string_type csa_move_resign = SC(I,"%%TORYO");
		const string_type csa_move_win = SC(I,"%%WIN");
		const string_type csa_cap_sq = SC(I,"00");
		const string_type csa_piece[16] = {
			SC(I,"**"), SC(I,"FU"), SC(I,"KY"), SC(I,"KE"), SC(I,"GI"), SC(I,"KA"), SC(I,"HI"), SC(I,"KI"),
			SC(I,"OU"), SC(I,"TO"), SC(I,"NY"), SC(I,"NK"), SC(I,"NG"), SC(I,"UM"), SC(I,"RY"), SC(I,"QU"),
		};
		const string_type csa_ver = SC(I,"V2.2");
		const string_type csa_comment = SC(I,"'");
		const string_type csa_pos_ini = SC(I,"PI");
		const string_type csa_pos_rank[9] = {
			SC(I,"P1"), SC(I,"P2"), SC(I,"P3"), SC(I,"P4"), SC(I,"P5"), SC(I,"P6"), SC(I,"P7"), SC(I,"P8"), SC(I,"P9"),
		};
		const string_type csa_pos_piece[32] = {
			SC(I," * "), SC(I,"+FU"), SC(I,"+KY"), SC(I,"+KE"), SC(I,"+GI"), SC(I,"+KA"), SC(I,"+HI"), SC(I,"+KI"),
			SC(I,"+OU"), SC(I,"+TO"), SC(I,"+NY"), SC(I,"+NK"), SC(I,"+NG"), SC(I,"+UM"), SC(I,"+RY"), SC(I,"+QU"),
			SC(I," * "), SC(I,"-FU"), SC(I,"-KY"), SC(I,"-KE"), SC(I,"-GI"), SC(I,"-KA"), SC(I,"-HI"), SC(I,"-KI"),
			SC(I,"-OU"), SC(I,"-TO"), SC(I,"-NY"), SC(I,"-NK"), SC(I,"-NG"), SC(I,"-UM"), SC(I,"-RY"), SC(I,"-QU"),
		};
		const string_type csa_hand_black = SC(I,"P+");
		const string_type csa_hand_white = SC(I,"P-");
		const string_type kif_color_black = SC(I,"▲");
		const string_type kif_color_white = SC(I,"△");
		const string_type kif_color_blackinv = SC(I,"▼");
		const string_type kif_color_whiteinv = SC(I,"▽");
		const string_type piece_color_black = SCU(I,"☗","▲");
		const string_type piece_color_white = SCU(I,"☖","△");
		const string_type piece_color_blackinv = SCU(I,"⛊","▼");
		const string_type piece_color_whiteinv = SCU(I,"⛉","▽");
		const string_type kif_move_none = SC(I,"エラー");
		const string_type kif_move_null = SC(I,"パス");
		const string_type kif_move_resign = SC(I,"投了");
		const string_type kif_move_win = SC(I,"勝ち宣言");
		const string_type kif_move_samepos = SC(I,"同");
		const string_type kif_fwsp = SC(I,"　");
		const string_type kif_move_drop = SC(I,"打");
		const string_type kif_move_not = SC(I,"不");
		const string_type kif_move_promote = SC(I,"成");
		const string_type kif_move_straight = SC(I,"直");
		const string_type kif_move_upper = SC(I,"上");
		const string_type kif_move_lower = SC(I,"引");
		const string_type kif_move_slide = SC(I,"寄");
		const string_type kif_move_left = SC(I,"左");
		const string_type kif_move_right = SC(I,"右");
		const string_type kif_lbrack = SC(I,"(");
		const string_type kif_rbrack = SC(I,")");
		const string_type char1to9_ascii[9] = {
			SC(I,"1"), SC(I,"2"), SC(I,"3"), SC(I,"4"), SC(I,"5"), SC(I,"6"), SC(I,"7"), SC(I,"8"), SC(I,"9"),
		};
		const string_type char1to9_kanji[9] = {
			SC(I,"一"), SC(I,"二"), SC(I,"三"), SC(I,"四"), SC(I,"五"), SC(I,"六"), SC(I,"七"), SC(I,"八"), SC(I,"九"),
		};
		const string_type char1to9_full_width_arabic[9] = {
			SC(I,"１"), SC(I,"２"), SC(I,"３"), SC(I,"４"), SC(I,"５"), SC(I,"６"), SC(I,"７"), SC(I,"８"), SC(I,"９"),
		};
		const string_type kif_piece[16] = {
			SC(I,"・"), SC(I,"歩"), SC(I,"香"), SC(I,"桂"), SC(I,"銀"), SC(I,"角"), SC(I,"飛"), SC(I,"金"),
			SC(I,"玉"), SC(I,"と"), SC(I,"成香"), SC(I,"成桂"), SC(I,"成銀"), SC(I,"馬"), SC(I,"龍"), SC(I,"女"),
		};
		const string_type bod_fline = SC(I,"  ９ ８ ７ ６ ５ ４ ３ ２ １");
		const string_type bod_hline = SC(I,"+---------------------------+");
		const string_type bod_vline = SC(I,"|");
		const string_type bod_rank[9] = {
			SC(I,"一"), SC(I,"二"), SC(I,"三"), SC(I,"四"), SC(I,"五"), SC(I,"六"), SC(I,"七"), SC(I,"八"), SC(I,"九"),
		};
		const string_type bod_piece[32] = {
			SC(I," ・"), SC(I," 歩"), SC(I," 香"), SC(I," 桂"), SC(I," 銀"), SC(I," 角"), SC(I," 飛"), SC(I," 金"),
			SC(I," 玉"), SC(I," と"), SC(I," 杏"), SC(I," 圭"), SC(I," 全"), SC(I," 馬"), SC(I," 龍"), SC(I," 女"),
			SC(I," ・"), SC(I,"v歩"), SC(I,"v香"), SC(I,"v桂"), SC(I,"v銀"), SC(I,"v角"), SC(I,"v飛"), SC(I,"v金"),
			SC(I,"v玉"), SC(I,"vと"), SC(I,"v杏"), SC(I,"v圭"), SC(I,"v全"), SC(I,"v馬"), SC(I,"v龍"), SC(I,"v女"),
		};
		const string_type bod_hand_color_black = SC(I,"先手の持駒：");
		const string_type bod_hand_color_white = SC(I,"後手の持駒：");
		const string_type bod_hand_piece[16] = {
			SC(I,"・"), SC(I,"歩"), SC(I,"香"), SC(I,"桂"), SC(I,"銀"), SC(I,"角"), SC(I,"飛"), SC(I,"金"),
			SC(I,"玉"), SC(I,"と"), SC(I,"杏"), SC(I,"圭"), SC(I,"全"), SC(I,"馬"), SC(I,"龍"), SC(I,"女"),
		};
		const string_type bod_hand_pad = SC(I," ");
		const string_type bod_hand_none = SC(I,"なし");
		const string_type bod_hand_num[19] = {
			SC(I,""), SC(I,""), SC(I,"二"), SC(I,"三"), SC(I,"四"), SC(I,"五"), SC(I,"六"), SC(I,"七"), SC(I,"八"), SC(I,"九"),
			SC(I,"十"), SC(I,"十一"), SC(I,"十二"), SC(I,"十三"), SC(I,"十四"), SC(I,"十五"), SC(I,"十六"), SC(I,"十七"), SC(I,"十八"),
		};
		const string_type bod_turn_black = SC(I,"先手番");
		const string_type bod_turn_white = SC(I,"後手番");
		const string_type kiflist_head = SC(I,"手数----指手---------消費時間--");
		const string_type kiflist_pad = SC(I," ");
		const string_type char0to9_ascii[10] = {
			SC(I,"0"),SC(I,"１"), SC(I,"２"), SC(I,"３"), SC(I,"４"), SC(I,"５"), SC(I,"６"), SC(I,"７"), SC(I,"８"), SC(I,"９"),
		};
	};

	struct KifConstLocale : KifConst<0, char>     {};
	struct KifConstUtf8   : KifConst<1, char>     {};
	struct KifConstUtf16  : KifConst<2, char16_t> {};
	struct KifConstUtf32  : KifConst<3, char32_t> {};
	struct KifConstWchar  : KifConst<4, wchar_t>  {};

#undef SC
#undef SCU

}

#endif // ifdef USE_KIF_CONVERT_TOOLS
