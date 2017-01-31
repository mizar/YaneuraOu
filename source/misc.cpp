﻿
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

#include <fstream>
#include <iomanip>
#include <iostream>
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
