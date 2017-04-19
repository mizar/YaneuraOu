
// Windows環境下でのプロセッサグループの割当関係
#ifdef _WIN32
#if _WIN32_WINNT < 0x0601
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Force to include needed API prototypes
#endif
#include <windows.h>
// The needed Windows API for processor groups could be missed from old Windows
// versions, so instead of calling them directly (forcing the linker to resolve
// the calls at compile time), try to load them at runtime. To do this we need
// first to define the corresponding function pointers.
extern "C" {
	typedef bool(*fun1_t)(LOGICAL_PROCESSOR_RELATIONSHIP,
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
	typedef bool(*fun2_t)(USHORT, PGROUP_AFFINITY);
	typedef bool(*fun3_t)(HANDLE, CONST GROUP_AFFINITY*, PGROUP_AFFINITY);
}

// このheaderのなかでmin,maxを定義してあって、C++のstd::min,maxと衝突して困る。
#undef max
#undef min

#endif

#include <codecvt>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

#include "misc.h"
#include "thread.h"

using namespace std;

// --------------------
//  統計情報
// --------------------

static int64_t hits[2], means[2];

void dbg_hit_on(bool b) { ++hits[0]; if (b) ++hits[1]; }
void dbg_mean_of(int v) { ++means[0]; means[1] += v; }

void dbg_print() {

  if (hits[0])
    cerr << "Total " << hits[0] << " Hits " << hits[1]
    << " hit rate (%) " << fixed << setprecision(3) << (100.0f * hits[1] / hits[0]) << endl;

  if (means[0])
    cerr << "Total " << means[0] << " Mean "
    << (double)means[1] / means[0] << endl;
}

// --------------------
//  Timer
// --------------------

Timer Time;

int Timer::elapsed() const { return int(Search::Limits.npmsec ? Threads.nodes_searched() : now() - startTime); }
int Timer::elapsed_from_ponderhit() const { return int(Search::Limits.npmsec ? Threads.nodes_searched()/*これ正しくないがこのモードでponder使わないからいいや*/ : now() - startTimeFromPonderhit); }

// --------------------
//  engine info
// --------------------

const string engine_info() {

  stringstream ss;
  
  ss << ENGINE_NAME << ' '
     << EVAL_TYPE_NAME << ' '
     << ENGINE_VERSION << setfill('0')
     << (Is64Bit ? " 64" : " 32")
     << TARGET_CPU << endl
     << "id author by yaneurao" << endl;

  return ss.str();
}

// --------------------
//  sync_out/sync_endl
// --------------------

std::ostream& operator<<(std::ostream& os, SyncCout sc) {
  static Mutex m;
  if (sc == IO_LOCK)    m.lock();
  if (sc == IO_UNLOCK)  m.unlock();
  return os;
}

// --------------------
//  logger
// --------------------

// logging用のhack。streambufをこれでhookしてしまえば追加コードなしで普通に
// cinからの入力とcoutへの出力をファイルにリダイレクトできる。
// cf. http://groups.google.com/group/comp.lang.c++/msg/1d941c0f26ea0d81
struct Tie : public streambuf
{
  Tie(streambuf* buf_ , streambuf* log_) : buf(buf_) , log(log_) {}

  int sync() { return log->pubsync(), buf->pubsync(); }
  int overflow(int c) { return write(buf->sputc((char)c), "<< "); }
  int underflow() { return buf->sgetc(); }
  int uflow() { return write(buf->sbumpc(), ">> "); }

  int write(int c, const char* prefix) {
    static int last = '\n';
    if (last == '\n')
      log->sputn(prefix, 3);
    return last = log->sputc((char)c);
  }

  streambuf *buf, *log; // 標準入出力 , ログファイル
};

struct Logger {
  static void start(bool b)
  {
    static Logger log;

    if (b && !log.file.is_open())
    {
      log.file.open("io_log.txt", ifstream::out);
      cin.rdbuf(&log.in);
      cout.rdbuf(&log.out);
      cout << "start logger" << endl;
    } else if (!b && log.file.is_open())
    {
      cout << "end logger" << endl;
      cout.rdbuf(log.out.buf);
      cin.rdbuf(log.in.buf);
      log.file.close();
    }
  }

private:
  Tie in, out;   // 標準入力とファイル、標準出力とファイルのひも付け
  ofstream file; // ログを書き出すファイル

  Logger() : in(cin.rdbuf(),file.rdbuf()) , out(cout.rdbuf(),file.rdbuf()) {}
  ~Logger() { start(false); }

};

void start_logger(bool b) { Logger::start(b); }

// --------------------
//  ファイルの丸読み
// --------------------

// ファイルを丸読みする。ファイルが存在しなくともエラーにはならない。空行はスキップする。
int read_all_lines(std::string filename, std::deque<std::string>& lines)
{
  fstream fs(filename,ios::in);
  if (fs.fail())
    return 1; // 読み込み失敗

  while (!fs.fail() && !fs.eof())
  {
    std::string line;
    getline(fs,line);
    if (line.length())
      lines.push_back(line);
  }
  fs.close();
  return 0;
}

// --------------------
//  prefetch命令
// --------------------

// prefetch命令を使わない。
#ifdef NO_PREFETCH

void prefetch(void*) {}

#else

void prefetch(void* addr) {

// SSEの命令なのでSSE2が使える状況でのみ使用する。
#ifdef USE_SSE2

#  if defined(__INTEL_COMPILER)
  // 最適化でprefetch命令を削除するのを回避するhack。MSVCとgccは問題ない。
  __asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
  _mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
  __builtin_prefetch(addr);
#  endif

#endif
}

#endif

// --------------------------------
//   char32_t -> utf-8 string 変換
// --------------------------------

namespace UniConv {

	// std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> だとLNK2001をVS2015,VS2017が吐く不具合の回避。
	// http://qiita.com/benikabocha/items/1fc76b8cea404e9591cf
	// https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error

#ifdef _MSC_VER // MSVCの場合
	std::wstring_convert<std::codecvt_utf8<uint32_t>, uint32_t> char32_utf8_converter;
#else // MSVC以外の場合
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> char32_utf8_converter;
#endif

	std::string char32_to_utf8string(const char32_t * r)
	{
#ifdef _MSC_VER // MSVCの場合
		return char32_utf8_converter.to_bytes((const uint32_t *)r);
#else // MSVC以外の場合
		return char32_utf8_converter.to_bytes(r);
#endif
	}

}

// --------------------
//   数値・文字列変換
// --------------------

namespace QConv {

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

	void u32toa(char * s, u32 i) { u32toa(&s, i); }
	void s32toa(char * s, s32 i) { s32toa(&s, i); }
	void u64toa(char * s, u64 i) { u64toa(&s, i); }
	void s64toa(char * s, s64 i) { s64toa(&s, i); }

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

	u32 atou32(const char * s) { return atou32(&s); }
	s32 atos32(const char * s) { return atos32(&s); }
	u64 atou64(const char * s) { return atou64(&s); }
	s64 atos64(const char * s) { return atos64(&s); }

}

// --------------------
//  全プロセッサを使う
// --------------------

namespace WinProcGroup {

#ifndef _WIN32

	void bindThisThread(size_t) {}

#else

	/// get_group() retrieves logical processor information using Windows specific
	/// API and returns the best group id for the thread with index idx. Original
	/// code from Texel by Peter Österlund.

	int get_group(size_t idx) {

		int threads = 0;
		int nodes = 0;
		int cores = 0;
		DWORD returnLength = 0;
		DWORD byteOffset = 0;

		// Early exit if the needed API is not available at runtime
		HMODULE k32 = GetModuleHandle(L"Kernel32.dll");
		auto fun1 = (fun1_t)GetProcAddress(k32, "GetLogicalProcessorInformationEx");
		if (!fun1)
			return -1;

		// First call to get returnLength. We expect it to fail due to null buffer
		if (fun1(RelationAll, nullptr, &returnLength))
			return -1;

		// Once we know returnLength, allocate the buffer
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, *ptr;
		ptr = buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);

		// Second call, now we expect to succeed
		if (!fun1(RelationAll, buffer, &returnLength))
		{
			free(buffer);
			return -1;
		}

		while (ptr->Size > 0 && byteOffset + ptr->Size <= returnLength)
		{
			if (ptr->Relationship == RelationNumaNode)
				nodes++;

			else if (ptr->Relationship == RelationProcessorCore)
			{
				cores++;
				threads += (ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
			}

			byteOffset += ptr->Size;
			ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((char*)ptr) + ptr->Size);
		}

		free(buffer);

		std::vector<int> groups;

		// Run as many threads as possible on the same node until core limit is
		// reached, then move on filling the next node.
		for (int n = 0; n < nodes; n++)
			for (int i = 0; i < cores / nodes; i++)
				groups.push_back(n);

		// In case a core has more than one logical processor (we assume 2) and we
		// have still threads to allocate, then spread them evenly across available
		// nodes.
		for (int t = 0; t < threads - cores; t++)
			groups.push_back(t % nodes);

		// If we still have more threads than the total number of logical processors
		// then return -1 and let the OS to decide what to do.
		return idx < groups.size() ? groups[idx] : -1;
	}


	/// bindThisThread() set the group affinity of the current thread

	void bindThisThread(size_t idx) {

		// If OS already scheduled us on a different group than 0 then don't overwrite
		// the choice, eventually we are one of many one-threaded processes running on
		// some Windows NUMA hardware, for instance in fishtest. To make it simple,
		// just check if running threads are below a threshold, in this case all this
		// NUMA machinery is not needed.
		if (Threads.size() < 8)
			return;

		// Use only local variables to be thread-safe
		int group = get_group(idx);

		if (group == -1)
			return;

		// Early exit if the needed API are not available at runtime
		HMODULE k32 = GetModuleHandle(L"Kernel32.dll");
		auto fun2 = (fun2_t)GetProcAddress(k32, "GetNumaNodeProcessorMaskEx");
		auto fun3 = (fun3_t)GetProcAddress(k32, "SetThreadGroupAffinity");

		if (!fun2 || !fun3)
			return;

		GROUP_AFFINITY affinity;
		if (fun2(group, &affinity))
			fun3(GetCurrentThread(), &affinity, nullptr);
	}

#endif

} // namespace WinProcGroup
