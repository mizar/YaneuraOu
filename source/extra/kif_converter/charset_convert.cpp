#if defined(USE_KIF_CONVERT_TOOLS)

#include <cstdint>
#include "charset_convert.h"

namespace KifConvertTools
{

	// SJIS妥当性検査
	// 1byte: (0x00-0x80 or 0xa0-0xdf or 0xfd-0xff)
	// 2byte: (0x81-0x9f or 0xe0-0xfc) (0x40-0x7e or 0x80-0xfc)
	bool sjis_validation(const char *& s, bool line)
	{
		unsigned char c = '\0';
		while ((c = *s) != '\0')
		{
			if (line && c == '\n') { ++s; return true; }
			if ((c > 0x80 && c < 0xa0) || (c > 0xdf && c < 0xfd))
			{
				// マルチバイト開始文字なら2バイト目をチェック
				c = *++s; if (c < 0x40 || c == 0x7f || c > 0xfc) return false;
			}
			// 次の文字のチェックへ
			++s;
		}
		return true;
	}

	// UTF8妥当性検査
	//         UTF8 BOM U+FEFF : 0xef      0xbb      0xbf
	//     U+0000 -     U+007F : 0x00-0x7f
	//     U+0080 -     U+07FF : 0xc2-0xdf 0x80-0xbf
	//     U+0800 -     U+0FFF : 0xe0      0xa0-0xbf 0x80-0xbf
	//     U+1000 -     U+FFFF : 0xe1-0xef 0x80-0xbf 0x80-0xbf
	//    U+10000 -    U+3FFFF : 0xf0      0x90-0xbf 0x80-0xbf 0x80-0xbf
	//    U+40000 -    U+FFFFF : 0xf1-0xf3 0x80-0xbf 0x80-0xbf 0x80-0xbf
	//   U+100000 -   U+10FFFF : 0xf4      0x80-0x8f 0x80-0xbf 0x80-0xbf
	// (以下はISO/IEC10646,RFC2279の旧符号化定義には存在するがUnicode5.2,RFC3629では定義外の範囲)
	//   U+110000 -   U+13FFFF : 0xf4      0x90-0xbf 0x80-0xbf 0x80-0xbf
	//   U+140000 -   U+1FFFFF : 0xf5-0xf7 0x80-0xbf 0x80-0xbf 0x80-0xbf
	//   U+200000 -   U+FFFFFF : 0xf8      0x88-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf
	//  U+1000000 -  U+3FFFFFF : 0xf9-0xfb 0x80-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf
	//  U+4000000 - U+3FFFFFFF : 0xfc      0x84-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf
	// U+40000000 - U+7FFFFFFF : 0xfd      0x80-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf 0x80-0xbf
	bool utf8_validation(const char *& s, bool line)
	{
		unsigned char c = '\0';
		while ((c = *s) != '\0')
		{
			if (line && c == '\n') { ++s; return true; }
			//     U+0000 -     U+007F : 0x00-0x7f
			if (c < 0x80);
			// 0x80-0xbf は 1バイト目に使う事がない（この範囲は2バイト目以降専用）
			// 0xc0-0xc1 を 1バイト目に使う事がない（U+0000 - U+007F は1バイトにて表記するので空く）
			else if (c < 0xc2) return false;
			//     U+0080 -     U+07FF : 0xc2-0xdf 0x80-0xbf
			else if (c < 0xe0) {
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			//     U+0800 -     U+0FFF : 0xe0      0xa0-0xbf 0x80-0xbf
			else if (c == 0xe0) {
				c = *++s; if (c < 0xa0 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			//     U+1000 -     U+FFFF : 0xe1-0xef 0x80-0xbf 0x80-0xbf
			else if (c < 0xf0) {
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			//    U+10000 -    U+3FFFF : 0xf0      0x90-0xbf 0x80-0xbf 0x80-0xbf
			else if (c == 0xf0) {
				c = *++s; if (c < 0x90 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			//    U+40000 -    U+FFFFF : 0xf1-0xf3 0x80-0xbf 0x80-0xbf 0x80-0xbf
			else if (c < 0xf4) {
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			//   U+100000 -   U+10FFFF : 0xf4      0x80-0x8f 0x80-0xbf 0x80-0xbf
			else if (c == 0xf4) {
				c = *++s; if (c < 0x80 || c > 0x8f) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
				c = *++s; if (c < 0x80 || c > 0xbf) return false;
			}
			// U+110000 - U+7FFFFFFF は扱わない
			else return false;
			// 次の文字のチェックへ
			++s;
		}
		return true;
	}

	// エンコード判別結果
	enum EncType : uint64_t {
		ENC_ILLEGAL = 0,
		ENC_ALL = 31,
		ENC_BIT_EMPTY = 1,
		ENC_BIT_SPACE = 2,
		ENC_BIT_ASCII = 4,
		ENC_BIT_SJIS = 8,
		ENC_BIT_UTF8 = 16,
		ENC_MASK_NOTEMPTY = 30,
		ENC_MASK_NOTSPACE = 28,
		ENC_MASK_NOTASCII = 24
	};
	EncType operator&(EncType L, EncType R)
	{
		return static_cast<EncType>(static_cast<uint64_t>(L) & static_cast<uint64_t>(R));
	}
	EncType operator|(EncType L, EncType R)
	{
		return static_cast<EncType>(static_cast<uint64_t>(L) | static_cast<uint64_t>(R));
	}
	bool enc_isempty(EncType enc) { return ((enc & ENC_BIT_EMPTY) != ENC_ILLEGAL); }
	bool enc_isspace(EncType enc) { return ((enc & ENC_BIT_SPACE) != ENC_ILLEGAL); }
	bool enc_isascii(EncType enc) { return ((enc & ENC_BIT_ASCII) != ENC_ILLEGAL); }
	bool enc_issjis(EncType enc) { return ((enc & ENC_BIT_SJIS) != ENC_ILLEGAL); }
	bool enc_isutf8(EncType enc) { return ((enc & ENC_BIT_UTF8) != ENC_ILLEGAL); }

	EncType check_encode(const char *& s, bool line)
	{
		EncType enc = ENC_ALL;
		unsigned char c = '\0';
		while ((c = *s) != '\0')
		{
			if (c < 0x21)
			{
				if (line && c == '\n')
				{
					++s;
					return enc;
				}
				else if (!line || c != '\r')
					enc = enc & ENC_MASK_NOTEMPTY;
			}
			else if (c < 0x80)
				enc = enc & ENC_MASK_NOTSPACE;
			else
			{
				enc = enc & ENC_MASK_NOTASCII;
				const char * s1 = s;
				const char * s2 = s;
				if (sjis_validation(s1, line)) s = s1; else enc = enc & ENC_BIT_UTF8;
				if (utf8_validation(s2, line)) s = s2; else enc = enc & ENC_BIT_SJIS;
				return enc;
			}
			// 次の文字のチェックへ
			++s;
		}
		return enc;
	}

}

#endif
