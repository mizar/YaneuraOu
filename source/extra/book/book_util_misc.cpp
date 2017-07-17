#include "book_util.h"

namespace BookUtil
{

	// ----------------
	//   変換・入出力関数類
	// ----------------

	// 数値 ←→ 文字列 変換

	// 内部実装

	void render_u16_4l(char *& s, u16 i)
	{
		u8 i0 = static_cast<u8>(i / static_cast<u16>(100u)), i1 = static_cast<u8>(i % static_cast<u16>(100u));
		*s++ = '0' + static_cast<char>(i0 / static_cast<u8>(10u));
		*s++ = '0' + static_cast<char>(i0 % static_cast<u8>(10u));
		*s++ = '0' + static_cast<char>(i1 / static_cast<u8>(10u));
		*s++ = '0' + static_cast<char>(i1 % static_cast<u8>(10u));
	}

	void render_u16_4u(char *& s, u16 i)
	{
		if (i < static_cast<u16>(10u))
			*s++ = '0' + static_cast<char>(i);
		else if (i < static_cast<u16>(100u))
		{
			*s++ = '0' + static_cast<char>(static_cast<u8>(i) / static_cast<u8>(10u));
			*s++ = '0' + static_cast<char>(static_cast<u8>(i) % static_cast<u8>(10u));
		}
		else
		{
			u8 i0 = static_cast<u8>(i / static_cast<u16>(100u)), i1 = static_cast<u8>(i % static_cast<u16>(100u));
			if (i < static_cast<u16>(1000u))
			{
				*s++ = '0' + static_cast<char>(i0);
				*s++ = '0' + static_cast<char>(i1 / static_cast<u8>(10u));
				*s++ = '0' + static_cast<char>(i1 % static_cast<u8>(10u));
			}
			else
			{
				*s++ = '0' + static_cast<char>(i0 / static_cast<u8>(10u));
				*s++ = '0' + static_cast<char>(i0 % static_cast<u8>(10u));
				*s++ = '0' + static_cast<char>(i1 / static_cast<u8>(10u));
				*s++ = '0' + static_cast<char>(i1 % static_cast<u8>(10u));
			}
		}
	}

	// 数値から文字列へ

	void u32toa(char *& s, u32 i)
	{
		if (i < static_cast<u32>(10000ul))
			render_u16_4u(s, static_cast<u16>(i));
		else if (i < static_cast<u32>(100000000ul))
		{
			render_u16_4u(s, static_cast<u16>(i / static_cast<u32>(10000ul)));
			render_u16_4l(s, static_cast<u16>(i % static_cast<u32>(10000ul)));
		}
		else
		{
			u32 i0 = i / static_cast<u32>(100000000ul), i1 = i % static_cast<u32>(100000000ul);
			render_u16_4u(s, static_cast<u16>(i0));
			u16 i10 = static_cast<u16>(i1 / static_cast<u32>(10000u)), i11 = static_cast<u16>(i1 % static_cast<u32>(10000u));
			render_u16_4l(s, static_cast<u16>(i10));
			render_u16_4l(s, static_cast<u16>(i11));
		}
		*s = '\0';
	}

	void s32toa(char *& s, s32 i)
	{
		if (i < 0)
		{
			*s++ = '-';
			u32toa(s, static_cast<u32>(-i));
		}
		else
			u32toa(s, static_cast<u32>(i));
	}

	void u64toa(char *& s, u64 i)
	{
		if (i < static_cast<u64>(10000ull))
			render_u16_4u(s, static_cast<s16>(i));
		else if (i < static_cast<u64>(100000000ul))
		{
			u16 i10 = static_cast<u16>(static_cast<u32>(i) / static_cast<u32>(10000u)), i11 = static_cast<u16>(static_cast<u32>(i) % static_cast<u32>(10000u));
			render_u16_4u(s, i10);
			render_u16_4l(s, i11);
		}
		else if (i < static_cast<u64>(10000000000000000ull))
		{
			u32 i0 = static_cast<u32>(i / static_cast<u64>(100000000ul)), i1 = static_cast<u32>(i % static_cast<u64>(100000000ul));
			if (i0 < static_cast<u32>(10000u))
			{
				u16 i10 = static_cast<u16>(i1 / static_cast<u32>(10000u)), i11 = static_cast<u16>(i1 % static_cast<u32>(10000u));
				render_u16_4u(s, static_cast<u16>(i0));
				render_u16_4l(s, i10);
				render_u16_4l(s, i11);
			}
			else
			{
				u16 i00 = static_cast<u16>(i0 / static_cast<u32>(10000u)), i01 = static_cast<u16>(i0 % static_cast<u32>(10000u));
				u16 i10 = static_cast<u16>(i1 / static_cast<u32>(10000u)), i11 = static_cast<u16>(i1 % static_cast<u32>(10000u));
				render_u16_4u(s, i00);
				render_u16_4l(s, i01);
				render_u16_4l(s, i10);
				render_u16_4l(s, i11);
			}
		}
		else
		{
			u64 iu = i / static_cast<u64>(10000000000000000ull), il = i % static_cast<u64>(10000000000000000ull);
			u32 i0 = static_cast<u32>(il / static_cast<u64>(100000000ul)), i1 = static_cast<u32>(il % static_cast<u64>(100000000ul));
			u16 i00 = static_cast<u16>(i0 / static_cast<u32>(10000u)), i01 = static_cast<u16>(i0 % static_cast<u32>(10000u));
			u16 i10 = static_cast<u16>(i1 / static_cast<u32>(10000u)), i11 = static_cast<u16>(i1 % static_cast<u32>(10000u));
			render_u16_4u(s, static_cast<u16>(iu));
			render_u16_4l(s, i00);
			render_u16_4l(s, i01);
			render_u16_4l(s, i10);
			render_u16_4l(s, i11);
		}
		*s = '\0';
	}

	void s64toa(char *& s, s64 i)
	{
		if (i < 0)
		{
			*s++ = '-';
			u64toa(s, static_cast<u64>(-i));
		}
		else
			u64toa(s, static_cast<u64>(i));
	}

	// 文字列から数値へ

	u32 atou32(const char *& s)
	{
		u32 ru32 = 0u;
		char c = *s;
		while (c == ' ') c = *++s;
		while (c == '0') c = *++s;
		if (c < '0' || c > '9') return ru32; else ru32 = static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru32;
		if (ru32 > static_cast<u32>(429496729lu)) goto _overlimit; else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if (ru32 < static_cast<u32>(UINT16_MAX)) goto _overlimit;
		if ((c = *++s) >= '0' && c <= '9') goto _overlimit;
		return ru32;
	_overlimit:;
		while ((c = *++s) >= '0' && c <= '9');
		return UINT32_MAX;
	}

	s32 atos32(const char *& s)
	{
		s32 rs32 = 0;
		bool minus = false;
		char c = *s;
		while (c == ' ') c = *++s;
		if (c == '-')
		{
			minus = true;
			c = *++s;
		}
		else if (c == '+')
			c = *++s;
		while (c == '0') c = *++s;
		if (c < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return minus ? -rs32 : rs32;
		if (rs32 > static_cast<s32>(214748364l)) goto _overlimit; else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if (rs32 < static_cast<s32>(0)) goto _overlimit;
		if ((c = *++s) >= '0' && c <= '9') goto _overlimit;
		return minus ? -rs32 : rs32;
	_overlimit:;
		while ((c = *++s) >= '0' && c <= '9');
		return minus ? INT32_MIN : INT32_MAX;
	}

	u64 atou64(const char *& s)
	{
		u32 ru32 = 0u;
		u64 ru64 = 0u;
		char c = *s;
		while (c == ' ') c = *++s;
		while (c == '0') c = *++s;
		if (c < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<u64>(ru32); else ru32 = ru32 * 10u + static_cast<u32>(c - '0');
		ru64 = static_cast<u64>(ru32);
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if ((c = *++s) < '0' || c > '9') return ru64;
		if (ru64 > static_cast<u64>(1844674407370955161ull)) goto _overlimit; else ru64 = ru64 * 10u + static_cast<u64>(c - '0');
		if (ru64 < static_cast<u64>(UINT32_MAX)) goto _overlimit;
		if ((c = *++s) >= '0' && c <= '9') goto _overlimit;
		return ru64;
	_overlimit:;
		while ((c = *++s) >= '0' && c <= '9');
		return UINT64_MAX;
	}

	s64 atos64(const char *& s)
	{
		s32 rs32 = 0;
		s64 rs64 = 0;
		bool minus = false;
		char c = *s;
		while (c == ' ') c = *++s;
		if (c == '-')
		{
			minus = true;
			c = *++s;
		}
		else if (c == '+')
			c = *++s;
		while (c == '0') c = *++s;
		if (c < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		if ((c = *++s) < '0' || c > '9') return static_cast<s64>(minus ? -rs32 : rs32); else rs32 = rs32 * 10 + static_cast<s32>(c - '0');
		rs64 = static_cast<s64>(rs32);
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if (rs64 < static_cast<s64>(0)) goto _overlimit;
		if ((c = *++s) >= '0' && c <= '9') goto _overlimit;
	_rs64:;
		return minus ? -rs64 : rs64;
	_overlimit:;
		while ((c = *++s) >= '0' && c <= '9');
		return minus ? INT64_MIN : INT64_MAX;
	}

	void _u32toa(char * s, u32 i) { u32toa(s, i); }
	void _s32toa(char * s, s32 i) { s32toa(s, i); }
	void _s64toa(char * s, s64 i) { s64toa(s, i); }
	void _u64toa(char * s, u64 i) { u64toa(s, i); }
	u32 _atou32(const char * s) { return atou32(s); }
	s32 _atos32(const char * s) { return atos32(s); }
	u64 _atou64(const char * s) { return atou64(s); }
	s64 _atos64(const char * s) { return atos64(s); }

	// Move 書き出し
	void movetoa(char *& s, const Move m)
	{
		if (!is_ok(m))
		{
			// 頻度が低いのでstringで手抜き
			std::string str;
			switch (m)
			{
			case MOVE_RESIGN:
				str = "resign";
				break;
			case MOVE_WIN:
				str = "win";
				break;
			case MOVE_NULL:
				str = "null";
				break;
			case MOVE_NONE:
				str = "none";
				break;
			default:;
			}
			size_t movelen = str.size();
			std::char_traits<char>::copy(s, str.c_str(), movelen);
			s += movelen;
		}
		else if (is_drop(m))
		{
			Square sq_to = move_to(m);
			*s++ = USI_PIECE[(m >> 6) & 30]; // == USIsIECE[(move_droppedsiece(m) & 15) * 2];
			*s++ = '*';
			*s++ = static_cast<char>('1' + file_of(sq_to));
			*s++ = static_cast<char>('a' + rank_of(sq_to));
		}
		else
		{
			Square sq_from = move_from(m), sq_to = move_to(m);
			*s++ = static_cast<char>('1' + file_of(sq_from));
			*s++ = static_cast<char>('a' + rank_of(sq_from));
			*s++ = static_cast<char>('1' + file_of(sq_to));
			*s++ = static_cast<char>('a' + rank_of(sq_to));
			if (is_promote(m))
				*s++ = '+';
		}
		*s = '\0'; // 念のためNULL文字出力
	}
	void _movetoa(char * s, const Move m) { movetoa(s, m); }

	// 文字列 → Move
	Move atomove(const char *& s)
	{
		char c0 = *s;
		if (c0 >= '1' && c0 <= '9')
		{
			char c1 = *++s; if (c1 < 'a' || c1 > 'i') goto _atomove_none;
			char c2 = *++s; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++s; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			char c4 = *++s;
			if (c4 == '+')
			{
				++s;
				return make_move_promote(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
			else
			{
				return make_move(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
		}
		else if (c0 >= 'A' && c0 <= 'Z')
		{
			char c1 = *++s; if (c1 != '*') goto _atomove_none;
			char c2 = *++s; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++s; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			++s;
			for (int i = 1; i <= 7; ++i)
				if (PieceToCharBW[i] == c0)
					return make_move_drop((Piece)i, toFile(c2) | toRank(c3));
		}
		else if (c0 == '0' || (c0 >= 'a' && c0 <= 'z'))
		{
			if (!memcmp(s, "win", 3)) { s += 3; return MOVE_WIN; }
			if (!memcmp(s, "null", 4)) { s += 4; return MOVE_NULL; }
			if (!memcmp(s, "pass", 4)) { s += 4; return MOVE_NULL; }
			if (!memcmp(s, "0000", 4)) { s += 4; return MOVE_NULL; }
		}
	_atomove_none:;
		while (*s >= '*') ++s;
		return MOVE_NONE;
	}
	Move _atomove(const char * p) { return atomove(p); }

	// BookEntry 書き出し
	void betoa(char *& s, const BookEntry & be)
	{
		// 先頭の"sfen "と末尾の指し手手数を除いた sfen文字列 の長さは最大で 95bytes?
		// "+l+n+sgkg+s+n+l/1+r5+b1/+p+p+p+p+p+p+p+p+p/9/9/9/+P+P+P+P+P+P+P+P+P/1+B5+R1/+L+N+SGKG+S+N+L b -"
		// 128bytes を超えたら明らかにおかしいので抑止。
		*s++ = 's';
		*s++ = 'f';
		*s++ = 'e';
		*s++ = 'n';
		*s++ = ' ';
		size_t sfenlen = std::min(be.sfenPos.size(), (size_t)128);
		std::char_traits<char>::copy(s, be.sfenPos.c_str(), sfenlen);
		s += sfenlen;
		*s++ = ' ';
		s32toa(s, be.ply);
		*s++ = '\n';
		// BookPosは改行文字を含めても 62bytes を超えないので、
		// be.move_list->size() <= 1000 なら64kiBの _buffer を食い尽くすことは無いはず
		// 1局面の合法手の最大は593（歩不成・2段香不成・飛不成・角不成も含んだ場合、以下局面例）なので、
		// sfen 8R/kSS1S1K2/4B4/9/9/9/9/9/3L1L1L1 b RB4GS4NL18P 1
		// be.move_list->size() > 1000 なら安全のため出力しない
		if (be.move_list->size() <= 1000)
			for (auto & bp : *be.move_list)
			{
				movetoa(s, bp.bestMove);
				*s++ = ' ';
				movetoa(s, bp.nextMove);
				*s++ = ' ';
				s32toa(s, bp.value);
				*s++ = ' ';
				s32toa(s, bp.depth);
				*s++ = ' ';
				u64toa(s, bp.num);
				*s++ = '\n';
			}
		*s = '\0'; // 念のためNULL文字出力
	}
	void _betoa(char * s, const BookEntry & be) { return betoa(s, be); }

	// char文字列 の BookPos 化
	void BookEntry::init(const char * sfen, const size_t length, const bool sfen_n11n, Thread* th)
	{
		if(sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(std::string(sfen, length), th);
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		} else
		{
			auto cur = trimlen_sfen(sfen, length);
			sfenPos = std::string(sfen, cur);
			ply = _atos32(sfen + cur);
		}
	}

	// ストリームからの BookEntry 読み込み
	void BookEntry::init(std::istream & is, char * , const size_t , const bool sfen_n11n, Thread* th)
	{
		std::string line;

		do
		{
			int c;
			// 行頭が's'になるまで読み飛ばす
			while ((c = is.peek()) != 's')
				if (c == EOF || !is.ignore(std::numeric_limits<std::streamsize>::max(), '\n'))
					return;
			// 行読み込み
			if (!getline(is, line))
				return;
			// "sfen "で始まる行は局面のデータであり、sfen文字列が格納されている。
			// 短すぎたり、sfenで始まらなかったりした行ならばスキップ
		} while (line.length() < 24 || line.compare(0, 5, "sfen ") != 0);
		if (sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(line.substr(5), th);
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		} else
		{
			auto sfen = split_sfen(line.substr(5));
			sfenPos = sfen.first;
			ply = sfen.second;
		}
	}

	// ストリームから BookEntry への BookPos 順次読み込み
	void BookEntry::incpos(std::istream & is, char * _buffer, const size_t _buffersize)
	{
		if (move_list.get() == nullptr)
			move_list = MoveListPtr(new MoveListType);
		while (true)
		{
			int c;
			// 行頭が数字か英文字以外なら行末文字まで読み飛ばす
			while ((c = is.peek(), (c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')))
				if (c == EOF || !is.ignore(std::numeric_limits<std::streamsize>::max(), '\n'))
					return;
			// 次のsfen文字列に到達していそうなら離脱（指し手文字列で's'から始まるものは無い）
			if (c == 's')
				return;
			// 行読み込み
			is.getline(_buffer, _buffersize - 8);
			// バッファから溢れたら行末まで読み捨て
			if (is.fail())
			{
				is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				is.clear(is.rdstate() & ~std::ios_base::failbit);
			}
			// BookPos 読み込み
			move_list->emplace_back(_buffer);
		}
	}

	// バイト文字列からの BookPos 読み込み
	void BookPos::init(const char * s)
	{
		// 解析実行
		// 0x00 - 0x1f, 0x21 - 0x29, 0x80 - 0xff の文字が現れ次第中止
		// 特に、NULL, CR, LF 文字に反応する事を企図。TAB 文字でも中止。SP 文字連続でも中止。
		if (*s < '*') return;
		bestMove = atomove(s);
		while (*s >= '*') ++s; if (*s != ' ' || *++s < '*') return;
		nextMove = atomove(s);
		while (*s >= '*') ++s; if (*s != ' ' || *++s < '*') return;
		value = atos32(s);
		while (*s >= '*') ++s; if (*s != ' ' || *++s < '*') return;
		depth = atos32(s);
		while (*s >= '*') ++s; if (*s != ' ' || *++s < '*') return;
		num = atou64(s);
	}

	// 出力ストリーム
	// write_book では最適化のため現在使われていない
	std::ostream & operator << (std::ostream & os, const BookPos & bp)
	{
		os << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << ' ' << bp.depth << ' ' << bp.num << "\n";
		return os;
	}
	std::ostream & operator << (std::ostream & os, const BookEntry & be)
	{
		os << "sfen " << be.sfenPos << " " << be.ply << "\n";
		for (auto & bp : *be.move_list)
			os << bp;
		return os;
	}

	// ----------------
	//   ユーティリティ関数類
	// ----------------

	// sfen文字列のゴミを除いた長さ
	size_t trimlen_sfen(const char * sfen, const size_t length)
	{
		auto cur = length;
		while (cur > 0)
		{
			char c = sfen[cur - 1];
			// 改行文字、スペース、数字(これはgame ply)ではないならループを抜ける。
			// これらの文字が出現しなくなるまで末尾を切り詰める。
			if (c != '\r' && c != '\n' && c != ' ' && !('0' <= c && c <= '9'))
				break;
			cur--;
		}
		return cur;
	}
	size_t trimlen_sfen(const std::string sfen)
	{
		return trimlen_sfen(sfen.c_str(), sfen.length());
	}

	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	const std::string trim_sfen(const std::string sfen)
	{
		std::string s = sfen;
		s.resize(trimlen_sfen(sfen));
		return s;
	}

	// sfen文字列の手数分離
	const sfen_pair_t split_sfen(const std::string sfen)
	{
		auto cur = trimlen_sfen(sfen);
		std::string s = sfen;
		s.resize(cur);
		int ply = _atos32(sfen.c_str() + cur);
		return make_pair(s, ply);
	}

	// 棋譜文字列ストリームから初期局面を抽出して設定
	std::string fetch_initialpos(Position & pos, std::istream & is, Thread* th)
	{
		std::string sfenpos, token;
		auto ispos = is.tellg();
		if (!(is >> token) || token == "moves")
			sfenpos = SFEN_HIRATE;
		else if (token != "position" && token != "sfen" && token != "startpos")
		{
			// 先頭句が予約語でないなら、いきなり平手からの指し手文字列が始まったと見なす
			sfenpos = SFEN_HIRATE;
			// シーク位置を戻す
			is.seekg(ispos);
		}
		else do
		{
			if (token == "moves")
				break;
			else if (token == "startpos")
				sfenpos = SFEN_HIRATE;
			else if (token == "sfen")
				sfenpos = "";
			else if (token != "position")
				sfenpos += token + " ";
		} while (is >> token);
		// 初期局面を設定
		pos.set(sfenpos, th);
		return sfenpos;
	}

}
