#if defined(USE_KIF_CONVERT_TOOLS)

namespace KifConvertTools
{
	// エンコーディング判別結果フラグ
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

	// エンコーディング判別
	EncType check_encode(const char *& s, bool line = false);
	// sjis -> utf8 変換
	void sjis2uni(const char * from, char *& to, bool line = false);
	// utf8 -> sjis 変換
	void uni2sjis(const char * from, char *& to, bool line = false);
}

#endif
