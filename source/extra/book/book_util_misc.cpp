#include "book_util.h"

namespace BookUtil
{

	// ----------------
	//   変換・入出力関数類
	// ----------------

	// 数値 ←→ 文字列 変換

	// 内部実装

	void render_u16_4l(char ** s, u16 i)
	{
		char * p = *s;
		u8 i0 = (u8)(i / (u16)100), i1 = (u8)(i % (u16)100);
		*p++ = '0' + (char)(i0 / (u8)10);
		*p++ = '0' + (char)(i0 % (u8)10);
		*p++ = '0' + (char)(i1 / (u8)10);
		*p++ = '0' + (char)(i1 % (u8)10);
		*s = p;
	}

	void render_u16_4u(char ** s, u16 i)
	{
		char * p = *s;
		if (i < (u16)10)
			*p++ = '0' + (char)i;
		else if (i < (u16)100)
		{
			*p++ = '0' + (char)((u8)i / (u8)10);
			*p++ = '0' + (char)((u8)i % (u8)10);
		}
		else
		{
			u8 i0 = (u8)(i / (u16)100), i1 = (u8)(i % (u16)100);
			if (i < (u16)1000)
			{
				*p++ = '0' + (char)i0;
				*p++ = '0' + (char)(i1 / (u8)10);
				*p++ = '0' + (char)(i1 % (u8)10);
			}
			else
			{
				*p++ = '0' + (char)(i0 / (u8)10);
				*p++ = '0' + (char)(i0 % (u8)10);
				*p++ = '0' + (char)(i1 / (u8)10);
				*p++ = '0' + (char)(i1 % (u8)10);
			}
		}
		*s = p;
	}

	// 数値から文字列へ

	void u32toa(char ** s, u32 i)
	{
		if (i < (u32)10000ul)
			render_u16_4u(s, (u16)i);
		else if (i < (u32)100000000ul)
		{
			render_u16_4u(s, (u16)(i / (u32)10000ul));
			render_u16_4l(s, (u16)(i % (u32)10000ul));
		}
		else
		{
			u32 i0 = i / (u32)100000000ul, i1 = i % (u32)100000000ul;
			render_u16_4u(s, (u16)i0);
			u16 i10 = (u16)(i1 / (u32)10000u), i11 = (u16)(i1 % (u32)10000u);
			render_u16_4l(s, (u16)i10);
			render_u16_4l(s, (u16)i11);
		}
		**s = '\0';
	}

	void s32toa(char ** s, s32 i)
	{
		if (i < 0)
		{
			*(*s)++ = '-';
			u32toa(s, (u32)-i);
		}
		else
			u32toa(s, (u32)i);
	}

	void u64toa(char ** s, u64 i)
	{
		if (i < (u64)10000ull)
			render_u16_4u(s, (s16)i);
		else if (i < (u64)100000000ul)
		{
			u16 i10 = (u16)((u32)i / (u32)10000u), i11 = (u16)((u32)i % (u32)10000u);
			render_u16_4u(s, i10);
			render_u16_4l(s, i11);
		}
		else if (i < (u64)10000000000000000ull)
		{
			u32 i0 = (u32)(i / (u64)100000000ul), i1 = (u32)(i % (u64)100000000ul);
			if (i0 < (u32)10000u)
			{
				u16 i10 = (u16)(i1 / (u32)10000u), i11 = (u16)(i1 % (u32)10000u);
				render_u16_4u(s, (u16)i0);
				render_u16_4l(s, i10);
				render_u16_4l(s, i11);
			}
			else
			{
				u16 i00 = (u16)(i0 / (u32)10000u), i01 = (u16)(i0 % (u32)10000u);
				u16 i10 = (u16)(i1 / (u32)10000u), i11 = (u16)(i1 % (u32)10000u);
				render_u16_4u(s, i00);
				render_u16_4l(s, i01);
				render_u16_4l(s, i10);
				render_u16_4l(s, i11);
			}
		}
		else
		{
			u64 iu = i / (u64)10000000000000000ull, il = i % (u64)10000000000000000ull;
			u32 i0 = (u32)(il / (u64)100000000ul), i1 = (u32)(il % (u64)100000000ul);
			u16 i00 = (u16)(i0 / (u32)10000u), i01 = (u16)(i0 % (u32)10000u);
			u16 i10 = (u16)(i1 / (u32)10000u), i11 = (u16)(i1 % (u32)10000u);
			render_u16_4u(s, (u16)iu);
			render_u16_4l(s, i00);
			render_u16_4l(s, i01);
			render_u16_4l(s, i10);
			render_u16_4l(s, i11);
		}
		**s = '\0';
	}

	void s64toa(char ** s, s64 i)
	{
		if (i < 0)
		{
			*(*s)++ = '-';
			u64toa(s, (u64)-i);
		}
		else
			u64toa(s, (u64)i);
	}

	// 文字列から数値へ

	u32 atou32(const char ** s)
	{
		const char * _s = *s;
		u32 ru32 = 0u;
		char c = *_s;
		while (c == ' ') c = *++_s;
		while (c == '0') c = *++_s;
		if (c < '0' || c > '9') goto _ru32; else ru32 = (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32;
		if (ru32 > (u32)429496729lu) goto _overlimit; else ru32 = ru32 * 10u + (u32)(c - '0');
		if (ru32 < (u32)UINT16_MAX) goto _overlimit;
		if ((c = *++_s) >= '0' && c <= '9') goto _overlimit;
	_ru32:;
		*s = _s;
		return ru32;
	_overlimit:;
		while ((c = *++_s) >= '0' && c <= '9');
		*s = _s;
		return UINT32_MAX;
	}

	s32 atos32(const char ** s)
	{
		const char * _s = *s;
		s32 rs32 = 0;
		bool minus = false;
		char c = *_s;
		while (c == ' ') c = *++_s;
		if (c == '-')
		{
			minus = true;
			c = *++_s;
		}
		else if (c == '+')
			c = *++_s;
		while (c == '0') c = *++_s;
		if (c < '0' || c > '9') goto _rs32; else rs32 = (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32;
		if (rs32 > (s32)214748364l) goto _overlimit; else rs32 = rs32 * 10 + (s32)(c - '0');
		if (rs32 < (s32)0) goto _overlimit;
		if ((c = *++_s) >= '0' && c <= '9') goto _overlimit;
	_rs32:;
		*s = _s;
		return minus ? -rs32 : rs32;
	_overlimit:;
		while ((c = *++_s) >= '0' && c <= '9');
		*s = _s;
		return minus ? INT32_MIN : INT32_MAX;
	}

	u64 atou64(const char ** s)
	{
		const char * _s = *s;
		u32 ru32 = 0u;
		u64 ru64 = 0u;
		char c = *_s;
		while (c == ' ') c = *++_s;
		while (c == '0') c = *++_s;
		if (c < '0' || c > '9') goto _ru32; else ru32 = (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru32; else ru32 = ru32 * 10u + (u32)(c - '0');
		ru64 = (u64)ru32;
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64; else ru64 = ru64 * 10u + (u64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _ru64;
		if (ru64 > (u64)1844674407370955161ull) goto _overlimit; else ru64 = ru64 * 10u + (u64)(c - '0');
		if (ru64 < (u64)UINT32_MAX) goto _overlimit;
		if ((c = *++_s) >= '0' && c <= '9') goto _overlimit;
	_ru64:;
		*s = _s;
		return ru64;
	_ru32:;
		*s = _s;
		return (u64)ru32;
	_overlimit:;
		while ((c = *++_s) >= '0' && c <= '9');
		*s = _s;
		return UINT64_MAX;
	}

	s64 atos64(const char ** s)
	{
		const char * _s = *s;
		s32 rs32 = 0;
		s64 rs64 = 0;
		bool minus = false;
		char c = *_s;
		while (c == ' ') c = *++_s;
		if (c == '-')
		{
			minus = true;
			c = *++_s;
		}
		else if (c == '+')
			c = *++_s;
		while (c == '0') c = *++_s;
		if (c < '0' || c > '9') goto _rs32; else rs32 = (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs32; else rs32 = rs32 * 10 + (s32)(c - '0');
		rs64 = (s64)rs32;
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if ((c = *++_s) < '0' || c > '9') goto _rs64; else rs64 = rs64 * 10 + (s64)(c - '0');
		if (rs64 < (s64)0) goto _overlimit;
		if ((c = *++_s) >= '0' && c <= '9') goto _overlimit;
	_rs64:;
		*s = _s;
		return minus ? -rs64 : rs64;
	_rs32:;
		*s = _s;
		return minus ? (s64)-rs32 : (s64)rs32;
	_overlimit:;
		while ((c = *++_s) >= '0' && c <= '9');
		*s = _s;
		return minus ? INT64_MIN : INT64_MAX;
	}

	void u32toa(char * s, u32 i) { u32toa(&s, i); }
	void s32toa(char * s, s32 i) { s32toa(&s, i); }
	void s64toa(char * s, s64 i) { s64toa(&s, i); }
	void u64toa(char * s, u64 i) { u64toa(&s, i); }
	u32 atou32(const char * s) { return atou32(&s); }
	s32 atos32(const char * s) { return atos32(&s); }
	u64 atou64(const char * s) { return atou64(&s); }
	s64 atos64(const char * s) { return atos64(&s); }

	// Move 書き出し
	void movetoa(char ** s, const Move m)
	{
		char * _s = *s;
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
			std::char_traits<char>::copy(_s, str.c_str(), movelen);
			_s += movelen;
		}
		else if (is_drop(m))
		{
			Square sq_to = move_to(m);
			*_s++ = USI_PIECE[(m >> 6) & 30]; // == USI_PIECE[(move_dropped_piece(m) & 15) * 2];
			*_s++ = '*';
			*_s++ = (char)('1' + file_of(sq_to));
			*_s++ = (char)('a' + rank_of(sq_to));
		}
		else
		{
			Square sq_from = move_from(m), sq_to = move_to(m);
			*_s++ = (char)('1' + file_of(sq_from));
			*_s++ = (char)('a' + rank_of(sq_from));
			*_s++ = (char)('1' + file_of(sq_to));
			*_s++ = (char)('a' + rank_of(sq_to));
			if (is_promote(m))
				*_s++ = '+';
		}
		*_s = '\0'; // 念のためNULL文字出力
		*s = _s;
	}
	void movetoa(char * s, const Move m) { movetoa(&s, m); }

	// 文字列 → Move
	Move atomove(const char ** p)
	{
		const char * _p = *p;
		char c0 = *_p;
		if (c0 >= '1' && c0 <= '9')
		{
			char c1 = *++_p; if (c1 < 'a' || c1 > 'i') goto _atomove_none;
			char c2 = *++_p; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++_p; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			char c4 = *++_p;
			if (c4 == '+')
			{
				*p = _p + 1;
				return make_move_promote(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
			else
			{
				*p = _p;
				return make_move(toFile(c0) | toRank(c1), toFile(c2) | toRank(c3));
			}
		}
		else if (c0 >= 'A' && c0 <= 'Z')
		{
			char c1 = *++_p; if (c1 != '*') goto _atomove_none;
			char c2 = *++_p; if (c2 < '1' || c2 > '9') goto _atomove_none;
			char c3 = *++_p; if (c3 < 'a' || c3 > 'i') goto _atomove_none;
			*p = _p + 1;
			for (int i = 1; i <= 7; ++i)
				if (PieceToCharBW[i] == c0)
					return make_move_drop((Piece)i, toFile(c2) | toRank(c3));
		}
		else if (c0 == '0' || (c0 >= 'a' && c0 <= 'z'))
		{
			if (!memcmp(_p, "win", 3)) { *p = _p + 3; return MOVE_WIN; }
			if (!memcmp(_p, "null", 4)) { *p = _p + 4; return MOVE_NULL; }
			if (!memcmp(_p, "pass", 4)) { *p = _p + 4; return MOVE_NULL; }
			if (!memcmp(_p, "0000", 4)) { *p = _p + 4; return MOVE_NULL; }
		}
	_atomove_none:;
		while (*_p >= '*') ++_p;
		*p = _p;
		return MOVE_NONE;
	}
	Move atomove(const char * p) { return atomove(&p); }

	// BookEntry 書き出し
	void betoa(char ** s, const BookEntry & be)
	{
		char * _s = *s;

		// 先頭の"sfen "と末尾の指し手手数を除いた sfen文字列 の長さは最大で 95bytes?
		// "+l+n+sgkg+s+n+l/1+r5+b1/+p+p+p+p+p+p+p+p+p/9/9/9/+P+P+P+P+P+P+P+P+P/1+B5+R1/+L+N+SGKG+S+N+L b -"
		// 128bytes を超えたら明らかにおかしいので抑止。
		*_s++ = 's';
		*_s++ = 'f';
		*_s++ = 'e';
		*_s++ = 'n';
		*_s++ = ' ';
		size_t sfenlen = std::min(be.sfenPos.size(), (size_t)128);
		std::char_traits<char>::copy(_s, be.sfenPos.c_str(), sfenlen);
		_s += sfenlen;
		*_s++ = ' ';
		s32toa(&_s, be.ply);
		*_s++ = '\n';
		// BookPosは改行文字を含めても 62bytes を超えないので、
		// be.move_list.size() <= 1000 なら64kiBの _buffer を食い尽くすことは無いはず
		// 1局面の合法手の最大は593（歩不成・2段香不成・飛不成・角不成も含んだ場合、以下局面例）なので、
		// sfen 8R/kSS1S1K2/4B4/9/9/9/9/9/3L1L1L1 b RB4GS4NL18P 1
		// be.move_list.size() > 1000 なら安全のため出力しない
		if (be.move_list.size() <= 1000)
			for (auto & bp : be.move_list)
			{
				movetoa(&_s, bp.bestMove);
				*_s++ = ' ';
				movetoa(&_s, bp.nextMove);
				*_s++ = ' ';
				s32toa(&_s, bp.value);
				*_s++ = ' ';
				s32toa(&_s, bp.depth);
				*_s++ = ' ';
				u64toa(&_s, bp.num);
				*_s++ = '\n';
			}
		*_s = '\0'; // 念のためNULL文字出力
		*s = _s;
	}
	void betoa(char * s, const BookEntry & be) { return betoa(&s, be); }

	// char文字列 の BookPos 化
	void BookEntry::init(const char * sfen, const size_t length, const bool sfen_n11n)
	{
		if(sfen_n11n)
		{
			// sfen文字列は手駒の表記に揺れがある。
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)
			// sfen()化しなおすことでやねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。
			Position pos;
			pos.set(std::string(sfen, length));
			sfenPos = pos.trimedsfen();
			ply = pos.game_ply();
		} else
		{
			auto cur = trimlen_sfen(sfen, length);
			sfenPos = std::string(sfen, cur);
			ply = atos32(sfen + cur);
		}
	}

	// ストリームからの BookEntry 読み込み
	void BookEntry::init(std::istream & is, char * , const size_t , const bool sfen_n11n)
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
			pos.set(line.substr(5));
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
			move_list.emplace_back(_buffer);
		}
	}

	// バイト文字列からの BookPos 読み込み
	void BookPos::init(const char * p)
	{
		// 解析実行
		// 0x00 - 0x1f, 0x21 - 0x29, 0x80 - 0xff の文字が現れ次第中止
		// 特に、NULL, CR, LF 文字に反応する事を企図。TAB 文字でも中止。SP 文字連続でも中止。
		if (*p < '*') return;
		bestMove = atomove(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		nextMove = atomove(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		value = atos32(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		depth = atos32(&p);
		while (*p >= '*') ++p; if (*p != ' ' || *++p < '*') return;
		num = atou64(&p);
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
		for (auto & bp : be.move_list)
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
		int ply = atos32(sfen.c_str() + cur);
		return make_pair(s, ply);
	}

	// 棋譜文字列ストリームから初期局面を抽出して設定
	std::string fetch_initialpos(Position & pos, std::istream & is)
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
		pos.set(sfenpos);
		return sfenpos;
	}

}
