#if defined(USE_KIF_CONVERT_TOOLS)

#include "peglib.h"

namespace KifConvertTools
{

	void ki2_parser(const char * s)
	{

		peg::Definition SJISCHAR, UTF8CHAR;

		SJISCHAR <= peg::cho(
			// 1byte: (0x00-0x80 or 0xa0-0xdf or 0xfd-0xff)
			peg::seq( peg::cls("\x20-\x80\xa0-\xdf\xfd-\xff") ),
			// 2byte: (0x81-0x9f or 0xe0-0xfc) (0x40-0x7e or 0x80-0xfc)
			peg::seq( peg::cls("\x81-\x9f\xe0-\xfc"), peg::cls("\x40-\x7e\x80-\xfc") )
		), [](const peg::SemanticValues& sv) { return std::string(sv.c_str()); };

		UTF8CHAR <= peg::cho(
			//     U+0000 -     U+007F : 0x00-0x7f
			peg::seq( peg::cls("\x20-\x7f") ),
			//     U+0080 -     U+07FF : 0xc2-0xdf 0x80-0xbf
			peg::seq( peg::cls("\xc2-\xdf"), peg::cls("\x80-\xbf") ),
			//     U+0800 -     U+0FFF : 0xe0      0xa0-0xbf 0x80-0xbf
			peg::seq( peg::cls("\xe0"     ), peg::cls("\xa0-\xbf"), peg::cls("\x80-\xbf") ),
			//     U+1000 -     U+FFFF : 0xe1-0xef 0x80-0xbf 0x80-0xbf
			peg::seq( peg::cls("\xe1-\xef"), peg::cls("\x80-\xbf"), peg::cls("\x80-\xbf") ),
			//    U+10000 -    U+3FFFF : 0xf0      0x90-0xbf 0x80-0xbf 0x80-0xbf
			peg::seq( peg::cls("\xf0"     ), peg::cls("\x90-\xbf"), peg::cls("\x80-\xbf"), peg::cls("\x80-\xbf") ),
			//    U+40000 -    U+FFFFF : 0xf1-0xf3 0x80-0xbf 0x80-0xbf 0x80-0xbf
			peg::seq( peg::cls("\xf1-\xf3"), peg::cls("\x80-\xbf"), peg::cls("\x80-\xbf"), peg::cls("\x80-\xbf") ),
			//   U+100000 -   U+10FFFF : 0xf4      0x80-0x8f 0x80-0xbf 0x80-0xbf
			peg::seq( peg::cls("\xf4"     ), peg::cls("\x80-\x8f"), peg::cls("\x80-\xbf"), peg::cls("\x80-\xbf") )
		), [](const peg::SemanticValues& sv) { return std::string(sv.c_str()); };

	}

}

#endif
