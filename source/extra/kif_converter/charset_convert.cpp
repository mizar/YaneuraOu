#if defined(USE_KIF_CONVERT_TOOLS)

#include <cstdint>
#include "charset_convert.h"
#include "charset_table.h"

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

	// SJIS → UTF8 変換
	void sjis2uni(const char *& sin, char *& sout, bool line)
	{
		unsigned char c = '\0';
		uint16_t cs = 0;
		uint32_t cu = 0;
		while ((c = *sin) != '\0')
		{
			if (c < 0x80)
			{
				*sout++ = c;
				++sin;
				if (line && c == '\n') break;
				continue;
			}
			if ((c > 0x80 && c < 0xa0) || (c > 0xdf && c < 0xfd))
			{
				cs = static_cast<uint16_t>(c) << 8;
				c = *++sin;
				if (c < 0x40 || c == 0x7f || c > 0xfc)
					break;
				cs += static_cast<uint16_t>(c);
			}
			else
			{
				cs = static_cast<uint16_t>(c);
			}
			++sin;
			auto itr = sjis2uni_map.find(cs);
			if (itr == sjis2uni_map.end())
			{
				*sout++ = '?';
				continue;
			}
			cu = itr->second;
			if (cu < 0x80u)
				*sout++ = static_cast<char>(cu);
			else if (cu < 0x800u)
			{
				*sout++ = static_cast<char>(0xc0u | (cu >> 6));
				*sout++ = static_cast<char>(0x80u | ((cu >> 0) & 0x3fu));
			}
			else if (cu < 0xffffu)
			{
				*sout++ = static_cast<char>(0xe0u | (cu >> 12));
				*sout++ = static_cast<char>(0x80u | ((cu >> 6) & 0x3fu));
				*sout++ = static_cast<char>(0x80u | ((cu >> 0) & 0x3fu));
			}
			else if (cu < 0x110000u)
			{
				*sout++ = static_cast<char>(0xf0u | (cu >> 18));
				*sout++ = static_cast<char>(0x80u | ((cu >> 12) & 0x3fu));
				*sout++ = static_cast<char>(0x80u | ((cu >> 6) & 0x3fu));
				*sout++ = static_cast<char>(0x80u | ((cu >> 0) & 0x3fu));
			}
			else
			{
				uint32_t cu0 = (cu & 0xffffu);
				if (cu0 < 0x800u)
				{
					*sout++ = static_cast<char>(0xc0u | (cu0 >> 6));
					*sout++ = static_cast<char>(0x80u | ((cu0 >> 0) & 0x3fu));
				}
				else
				{
					*sout++ = static_cast<char>(0xe0u | (cu0 >> 12));
					*sout++ = static_cast<char>(0x80u | ((cu0 >> 6) & 0x3fu));
					*sout++ = static_cast<char>(0x80u | ((cu0 >> 0) & 0x3fu));
				}
				uint32_t cu1 = (cu >> 16);
				if (cu1 < 0x800u)
				{
					*sout++ = static_cast<char>(0xc0u | (cu1 >> 6));
					*sout++ = static_cast<char>(0x80u | ((cu1 >> 0) & 0x3fu));
				}
				else
				{
					*sout++ = static_cast<char>(0xe0u | (cu1 >> 12));
					*sout++ = static_cast<char>(0x80u | ((cu1 >> 6) & 0x3fu));
					*sout++ = static_cast<char>(0x80u | ((cu1 >> 0) & 0x3fu));
				}
			}
		}
		*sout = '\0';
	}

	// UTF8 → SJIS 変換
	void uni2sjis(const char *& sin, char *& sout, bool line)
	{
		unsigned char c = '\0';
		uint32_t cu = 0;
		uint16_t cs = 0;
		while ((c = *sin) != '\0')
		{
			if (c < 0x80)
			{
				*sout++ = c;
				++sin;
				if (line && c == '\n') break;
				continue;
			}
			else if (c < 0xc2)
				break;
			else if (c < 0xe0)
			{
				cu = (static_cast<uint32_t>(c & 0x1fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else if (c == 0xe0)
			{
				cu = (static_cast<uint32_t>(c & 0xfu) << 12);
				c = *++sin; if (c < 0xa0 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else if (c < 0xf0)
			{
				cu = (static_cast<uint32_t>(c & 0xfu) << 12);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else if (c == 0xf0)
			{
				cu = (static_cast<uint32_t>(c & 0x7u) << 18);
				c = *++sin; if (c < 0x90 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 12);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else if (c < 0xf4)
			{
				cu = (static_cast<uint32_t>(c & 0x7u) << 18);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 12);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else if (c == 0xf4)
			{
				cu = (static_cast<uint32_t>(c & 0x7u) << 18);
				c = *++sin; if (c < 0x80 || c > 0x8f) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 12);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 6);
				c = *++sin; if (c < 0x80 || c > 0xbf) break;
				cu |= (static_cast<uint32_t>(c & 0x3fu) << 0);
			}
			else
				break;
			++sin;
			auto itr = uni2sjis_map.find(cu);
			if (itr == uni2sjis_map.end())
			{
				*sout++ = '?';
				continue;
			}
			cs = itr->second;
			if (cs < 0x80u)
			{
				*sout++ = static_cast<char>(cs);
			}
			else
			{
				*sout++ = static_cast<char>(cs >> 8);
				*sout++ = static_cast<char>(cs & 0xffu);
			}
		}
		*sout = '\0';
	}

}

#endif
