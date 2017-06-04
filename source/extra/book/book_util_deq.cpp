#include "book_util.h"

namespace BookUtil
{

	// std::dequeを使った実装
	// std::unordered_map では順序関係が破壊されるが、 std::map は敢えて使わなかった的な。
	// 定跡同士のマージを行いやすくする（先頭要素のpopを多用する）ため、 std::vector ではなく std::deque を用いた。
	// 要素整列の戦略はTimSortの処理中の状態を参考に、必ずしも常にに全体を整列済みの状態には保たないものの、
	// ある程度要素追加中の二分探索も行いやすく（先に追加した要素ほど大きな整列済み区間に含まれる）、
	// 必要な際には低コストで全体を整列でき、元データがほぼ整列済みであればそれを利用する、
	// ただし実装簡易化のため逆順ソート済みだった区間をreverseする戦略は取らない
	//   { = 逆順ソート済み区間の整列は頻出しないと踏んで O(n) ではなく O(n log n) で妥協 }、
	// メモリ消費のオーバーヘッドはstd::mapより少なくする、などを目指す。

	// find() で見つからなかった時の値
	const BookIter deqBook::end()
	{
		return book_body.end();
	}

	BookIter deqBook::find(const BookEntry & be)
	{
		auto it0 = book_body.begin();
		auto itr = book_run.begin();
		while (true)
		{
			auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
			auto itf = std::lower_bound(it0, it1, be, less_bebe());
			if (itf != it1 && *itf == be)
				return itf;
			if (itr == book_run.end())
				return end();
			++itr;
			it0 = it1;
		}
	}
	BookIter deqBook::find(const std::string & sfenPos)
	{
		auto it0 = book_body.begin();
		auto itr = book_run.begin();
		while (true)
		{
			auto it1 = (itr != book_run.end()) ? std::next(book_body.begin(), *itr) : book_body.end();
			auto itf = std::lower_bound(it0, it1, sfenPos, less_bestr());
			if (itf != it1 && (*itf).sfenPos == sfenPos)
				return itf;
			if (itr == book_run.end())
				return end();
			++itr;
			it0 = it1;
		}
	}
	BookIter deqBook::find(const Position & pos)
	{
		return find(pos.trimedsfen());
	}

	// 部分的マージソート（下記[1][2]が満たされるまで最後の2つの区間をマージソートする）
	// [1] ソート済み区間が2つ以上ある時、最後の区間は最後から2つ目の区間より小さい
	// [2] ソート済み区間が3つ以上ある時、(最後の区間+最後から2つ目の区間)は最後から3つ目の区間より小さい
	void deqBook::intl_merge_part()
	{
		bool f = !book_run.empty();
		auto end_d = distance(book_body.begin(), book_body.end());
		while (f)
		{
			f = false;
			auto run_size = book_run.size();
			BookIterDiff run_back2, run_back;
			BookType::iterator body_begin;
			switch (run_size)
			{
			case 1:
				run_back = book_run.back();
				if (run_back <= end_d - run_back)
				{
					body_begin = book_body.begin();
					inplace_merge(body_begin, next(body_begin, run_back), book_body.end());
					book_run.pop_back();
				}
				break;
			case 2:
				run_back2 = book_run.front();
				run_back = book_run.back();
				if (run_back - run_back2 <= end_d - run_back)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					f = true;
				}
				else if (run_back2 <= end_d - run_back2)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					body_begin = book_body.begin();
					inplace_merge(body_begin, next(body_begin, run_back2), book_body.end());
					book_run.pop_back();
				}
				break;
			default:
				run_back2 = book_run[run_size - 2];
				run_back = book_run.back();
				if (
					run_back - run_back2 <= end_d - run_back ||
					run_back2 - book_run[run_size - 3] <= end_d - run_back2
					)
				{
					body_begin = book_body.begin();
					inplace_merge(next(body_begin, run_back2), next(body_begin, run_back), book_body.end());
					book_run.pop_back();
					f = true;
				}
			}
		}
	}

	// マージソート(全体)
	void deqBook::intl_merge()
	{
		while (!book_run.empty())
		{
			BookIterDiff d1 = book_run.back();
			book_run.pop_back();
			BookIterDiff d0 = book_run.empty() ? (BookIterDiff)0 : book_run.back();
			auto body_begin = book_body.begin();
			inplace_merge(next(body_begin, d0), next(body_begin, d1), book_body.end());
		}
	}

	// 同一局面の整理
	// 大量に局面登録する際、毎度重複チェックを行うよりまとめて処理する方が？
	void deqBook::intl_uniq()
	{
		// あらかじめ全体をソートしておく
		intl_merge();

		if (book_body.empty()) return;
		BookType::iterator it0 = book_body.begin(), it1 = next(it0), end = book_body.end();
		while (it1 != end)
			if (*it0 == *it1)
			{
				do
					(*it0).select(std::move(*it1++));
				while (it1 != end && *it0 == *it1);
				it0 = book_body.erase(++it0, it1);
				end = book_body.end();
				it1 = next(it0);
			}
			else
			{
				++it0;
				++it1;
			}
	}

	// 局面の追加
	// 大量の局面を追加する場合、重複チェックを逐一は行わず(dofind_false)に、後で intl_uniq() を行うことを推奨
	// 返り値 0:新規局面追加 1:登録済み局面有り
	int deqBook::add(BookEntry & be, bool dofind)
	{
		BookType::iterator it;
		int cmp;
		if (dofind && (it = find(be)) != end())
		{
			// 同一局面が存在したら整理して再登録
			(*it).select(be);
			return 1;
		}
		else if (book_body.empty() || (cmp = be.compare(book_body.back())) > 0)
			// 順序関係が保たれているなら単純に末尾に追加
			book_body.push_back(be);
		else if (cmp == 0)
		{
			// 末尾と同一局面なら整理
			book_body.back().select(be);
			return 1;
		}
		else if (book_run.empty())
			if (book_body.size() < MINRUN)
				// 短ければ、挿入ソート
				book_body.insert(upper_bound(book_body.begin(), book_body.end(), be), be);
			else
			{
				// 長ければ、新しいソート済み範囲を追加
				book_run.push_back(distance(book_body.begin(), book_body.end()));
				book_body.push_back(be);
			}
		else
		{
			if (distance(next(book_body.begin(), book_run.back()), book_body.end()) < (BookIterDiff)MINRUN)
				// 短ければ、挿入ソート
				book_body.insert(upper_bound(next(book_body.begin(), book_run.back()), book_body.end(), be), be);
			else
			{
				// 長ければ、部分的なマージソートを試みてから
				intl_merge_part();
				// 新しいソート済み範囲を追加
				book_run.push_back(distance(book_body.begin(), book_body.end()));
				book_body.push_back(be);
			}
		}
		return 0;
	}

	// 局面・指し手の追加
	void deqBook::insert_book_pos(const sfen_pair_t & sfen, BookPos & bp)
	{
		auto it = find(sfen.first);
		if (it == end())
		{
			// 存在しないので要素を作って追加。
			BookEntry be(sfen);
			be.move_list.push_back(bp);
			add(be);
		}
		else
			// この局面での指し手のリスト
			it->insert_book_pos(bp);
	}

	void deqBook::clear()
	{
		book_body.clear();
		book_run.clear();
	}

	// 定跡ファイルの読み込み(book.db)など。
	int deqBook::read_book(const std::string & filename, bool sfen_n11n)
	{
		// 読み込み済であるかの判定
		if (book_name == filename)
			return 0;

		// 別のファイルを開こうとしているなら前回メモリに丸読みした定跡をクリアしておかないといけない。
		if (book_name != "")
			clear();

		// 1行ごとの読み込みはオーバーヘッドが大きいので、作業領域を介して読み込む

		// ファイルハンドラ
		std::ifstream fs;

		// 読み込みバッファ
		const size_t buffersize = 16 * 1024 * 1024; // 16MiB
		char * buffer = new char[buffersize];
		// 読み込み範囲始点、読み込み範囲終点、バッファ内最終行始点、バッファ終点
		size_t idx0 = 0, idx1 = 0, lastheadidx = 0, endidx = 0;

		// バッファへの次ブロック読み込み
		auto readblock = [&]()
		{
			size_t _idx = 0, _remain = 0;
			// 読み残し領域を先頭にコピー
			if (idx0 < endidx)
				memcpy(buffer, buffer + idx0, _idx = endidx - idx0);
			idx0 = idx1 = 0;
			// バッファの残りサイズ
			_remain = buffersize - _idx;
			// 1文字分の余裕を残して読み込み、空いた1文字はNULで埋める
			endidx = _idx + (_remain > 1 ? (size_t)(fs.read(buffer + _idx, _remain - 1).gcount()) : 0);
			char * _p = buffer + endidx;
			*_p = '\0';
			// 最終行の行頭位置検出
			size_t _lastheadidx = endidx;
			for (; _lastheadidx > 0 && *--_p != '\n'; --_lastheadidx);
			// 全く改行文字が存在しなければ、行末まで読み捨てる
			if (_lastheadidx == 0 && *buffer != '\n')
				fs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			lastheadidx = _lastheadidx;
		};

		// バッファ内の次行範囲探索、必要に応じてバッファへの次ブロック読み込み
		auto nextrange = [&]()
		{
			// 前回の終端を始点として次の行頭位置を調べる
			size_t _idx0 = idx1, _lastheadp0 = lastheadidx;
			char * _p0 = buffer + _idx0;
			while (_idx0 < _lastheadp0 && *_p0++ == '\n') ++_idx0;
			idx0 = _idx0;
			// バッファを読み終わるなら次ブロックをバッファに読み込み
			if (_idx0 >= _lastheadp0)
				readblock();
			// 行末位置を調べる
			size_t _idx1 = idx0, _lastheadp1 = lastheadidx;
			char * _p1 = buffer + _idx1;
			while (_idx1 < _lastheadp1 && *_p1++ != '\n') ++_idx1;
			idx1 = _idx1;
		};

		fs.open(filename, std::ifstream::in | std::ifstream::binary);
		if (fs.bad() || !fs.is_open())
		{
			std::cout << "info string Error! : can't read " + filename << std::endl;
			return 1;
		}

		// 先頭行読み込み
		nextrange();
		while (idx1 > 0)
		{
			// "sfen " で始まる25バイト以上の行を見つける
			if (memcmp(buffer + idx0, "sfen ", 5) || idx0 + 24 > idx1)
			{
				nextrange();
				continue;
			}
			// BookEntry 構築
			BookEntry be(buffer + idx0 + 5, idx1 - idx0 - 5, sfen_n11n);
			// BookEntry への BookPos要素充填
			while (true)
			{
				nextrange();
				// ファイル終端に達したら終了
				if (idx1 == 0)
					break;
				// 次の sfen が始まったら終了
				if (!memcmp(buffer + idx0, "sfen ", 5))
					break;
				// 行頭文字がおかしければスキップして次の行を調べる
				char c = buffer[idx0];
				if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
					continue;
				// BookEntry に BookPos 構築 & 追加
				be.move_list.emplace_back(buffer + idx0);
			}
			// おかしければ登録せずに次へ
			if (be.sfenPos.empty() || be.move_list.empty())
				continue;
			// 採択回数でsortしておく
			// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
			be.sort_pos();
			// dMemoryBook に登録
			add(be);
		}

		fs.close();

		delete[] buffer;

		// 全体のソート、重複局面の除去
		intl_uniq();

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book_name = filename;

		return 0;
	}

	// 定跡ファイルの書き出し
	int deqBook::write_book(const std::string & filename)
	{
		// 全体のソートと重複局面の除去
		intl_uniq();

		// 1行ごとの書き出しはオーバーヘッドが大きいので、作業領域を介して書き出す

		// 1要素あたりの最大サイズ
		const size_t entrymaxsize = 65536; // 64kiB
		// 領域確保係数
		const size_t renderbufmulti = 256;
		// 最大バッファ領域サイズ
		const size_t renderbufmaxsize = entrymaxsize * renderbufmulti; // 16MiB
		// バッファ領域サイズ
		size_t renderbufsize = entrymaxsize * std::min(renderbufmulti, book_body.size() + 1);
		// バッファ払い出し閾値
		ptrdiff_t renderbufth = (ptrdiff_t)renderbufsize - (ptrdiff_t)entrymaxsize;
		// バッファ確保
		char * renderbuf = new char[renderbufsize];

		// 文字列バッファ char[] に BookEntry を書き出し
		// BookEntry 毎に書き出しを行う
		char * p;

		std::ofstream fs;
		fs.open(filename, std::ofstream::out);
		if (fs.bad() || !fs.is_open())
		{
			std::cout << "info string Error! : can't write " + filename << std::endl;
			return 1;
		}

		// バージョン識別用文字列
		fs << "#YANEURAOU-DB2016 1.00" << std::endl;

		*(p = renderbuf) = '\0';
		for (BookEntry & be : book_body)
		{
			// 指し手のない空っぽのentryは書き出さないように。
			if (be.move_list.empty()) continue;
			// 採択回数でソートしておく。
			be.sort_pos();
			// 出力
			betoa(&p, be);
			// _bufth (16320kiB) を超えて出力が溜まったらファイルに書き出す
			if (p - renderbuf > renderbufth)
			{
				fs.write(renderbuf, p - renderbuf);
				*(p = renderbuf) = '\0';
			}
		}
		// 残りの出力をファイルに書き出し
		if (p != renderbuf)
			fs.write(renderbuf, p - renderbuf);

		fs.close();

		// バッファ開放
		delete[] renderbuf;

		return 0;
	}

	int deqBook::close_book()
	{
		clear();
		return 0;
	}

	MoveListTypeOpt deqBook::get_entries(const Position & pos)
	{
		auto it = find(pos);
		if (it == end())
			return {};
		else
		{
			move_update(pos, it->move_list);
			return it->move_list;
		}
	}

}
