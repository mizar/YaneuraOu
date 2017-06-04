<!-- Markdown -->
# 定跡ツール(bookutil)拡張コマンドまとめ (2017Early + work_book fork)

定跡関連、あわせて参照:
- [./解説.txt](./解説.txt)
- [./やねうら大定跡.txt](./やねうら大定跡.txt)
- [将棋ソフト用の標準定跡ファイルフォーマットの提案(2016-02-05)](http://yaneuraou.yaneu.com/2016/02/05/%E5%B0%86%E6%A3%8B%E3%82%BD%E3%83%95%E3%83%88%E7%94%A8%E3%81%AE%E6%A8%99%E6%BA%96%E5%AE%9A%E8%B7%A1%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88%E3%81%AE/)
- [やねうら大定跡はじめました(2016-07-10)](http://yaneuraou.yaneu.com/2016/07/10/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E5%A4%A7%E5%AE%9A%E8%B7%A1%E3%81%AF%E3%81%98%E3%82%81%E3%81%BE%E3%81%97%E3%81%9F/)
- [やねうら王を使って定跡を作る(2016-07-11)](http://yaneuraou.yaneu.com/2016/07/11/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8B%E3%82%92%E4%BD%BF%E3%81%A3%E3%81%A6%E5%AE%9A%E8%B7%A1%E3%82%92%E4%BD%9C%E3%82%8B/)
- [やねうら大定跡V1、公開しました。(2016-07-14)](http://yaneuraou.yaneu.com/2016/07/14/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E5%A4%A7%E5%AE%9A%E8%B7%A1v1%E3%80%81%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F%E3%80%82/)
- [自己対局用に互角の局面集を公開しました(2016-08-24)](http://yaneuraou.yaneu.com/2016/08/24/%E8%87%AA%E5%B7%B1%E5%AF%BE%E5%B1%80%E7%94%A8%E3%81%AB%E4%BA%92%E8%A7%92%E3%81%AE%E5%B1%80%E9%9D%A2%E9%9B%86%E3%82%92%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F/)

## 外部で生成されたやねうら棋譜定跡ファイルの取り込み

- sfen文字列（の持ち駒表記）が正規化されていない
  - [sfen文字列は一意に定まらない件(2016-07-15)](http://yaneuraou.yaneu.com/2016/07/15/sfen%E6%96%87%E5%AD%97%E5%88%97%E3%81%AF%E4%B8%80%E6%84%8F%E3%81%AB%E5%AE%9A%E3%81%BE%E3%82%89%E3%81%AA%E3%81%84%E4%BB%B6/)
  - [sfen文字列は本来は一意に定まる件(2016-07-15)](http://yaneuraou.yaneu.com/2016/07/15/sfen%E6%96%87%E5%AD%97%E5%88%97%E3%81%AF%E6%9C%AC%E6%9D%A5%E3%81%AF%E4%B8%80%E6%84%8F%E3%81%AB%E5%AE%9A%E3%81%BE%E3%82%8B%E4%BB%B6/)
- sfen文字列による辞書順に局面が並んでいない
  - 辞書順に並んでいないと BookOnTheFly の機能が使えない
- 手数のみ異なる同一局面が複数含まれる

このような定跡ファイルを正規化したい場合に、 `bookutil sort` コマンドを使う。

2017Early + work_book fork のバージョンでは、`bookutil {from_sfen | think | merge}` コマンドの出力は辞書順に出力されるため、必ずしも `bookutil sort` コマンドによる後処理は必要ない。（sfen文字列の持ち駒表記が正規化されていない定跡ファイルを扱った場合を除く）

## bookutil コマンドリファレンス

```
bookutil from_sfen <sfen_filename> <book_filename> [(options)...]
bookutil from_sfen bw <black_sfen_filename> <white_sfen_filename> <book_filename> [(options)...]
options:
  [moves <value>] : 最大手数(default: 16)
  [blackonly] : black手番の局面のみ登録
  [whiteonly] : white手番の局面のみ登録

bookutil think <sfen_filename> <book_filename> [(options)...]
bookutil think bw <black_sfen_filename> <white_sfen_filename> <book_filename> [(options)...]
options:
  [moves <value>] : 最大手数(default: 16)
  [depth <value>] : 探索深さ(default: 24)
  [startmoves <value>] : 開始手数(default: 1)
  [cluster <id> <num>] : クラスタ用間引き指定(default: 1 1)
  [progresstype <value>] : 思考中経過表示のタイプ(default: 0)
    0:標準 1:冗長
  [blackonly] : black手番の局面のみ思考して登録
  [whiteonly] : white手番の局面のみ思考して登録

bookutil {to_sfen | to_kif1 | to_kif2 | to_csa1 | to_csa} <book_filename> <sfen_filename> [(options)...]
options:
  [moves <value>] : 最大手数(default: 256)
  [evaldiff <value>] : 候補手分岐幅（最善手からの評価値の差）
  [evalblackdiff <value>] : 候補手分岐幅
    黒≒先手/下手、「９９」のマス目に近い側
    (default: Options["BookEvalDiff"])
  [evalwhitediff <value>] : 候補手分岐幅
    白≒後手/上手、「１１」のマス目に近い側
    (default: Options["BookEvalDiff"])
  [evallimit <value>] : 評価値制限
  [evalblacklimit <value>] : 評価値制限
    黒≒先手/下手、「９９」のマス目に近い側
    (default: Options["BookEvalBlackLimit"])
  [evalwhitelimit <value>] : 評価値制限
    白≒後手/上手、「１１」のマス目に近い側
    (default: Options["BookEvalWhiteLimit"])
  [depthlimit <value>] : 探索深さ制限
    (default: Options["BookDepthLimit"])
  [squareformat <value>] : マス目表記(to_kif1, to_kif2)
    0:"55" 1:"５５" 2:"５五"
    (default: 0)
  [opening <quoted_string>] : 開始局面・手順
    sfen形式、引用符""で囲む、複数指定時は引用符中でカンマ","区切り
    (default: "")
  [comment] : 末尾にコメント付与
    "#UNFILLED" : 未登録局面到達
    "#DUPEPOS" : 重複局面到達（既出局面に合流、または千日手模様）
    "#EVALLIMIT" : 評価値制限
    "#MOVELIMIT" : 手数制限
    "#DEPTHLIMIT" : 探索深さ制限
    "#EMPTYLIST" : 候補手未登録

bookutil merge <import_book0_filename> <import_book1_filename> <export_book_filename>
  import_book0 と import_book1 に登録された局面をマージ（重複する局面はより深い探索の方を採用）
bookutil softfilter <import_book_filename> <filter_book_filename> <export_book_filename>
  import_book を filter_book に登録された局面のみに削ぎ落とす
bookutil hardfilter <import_book_filename> <filter_book_filename> <export_book_filename>
  import_book を filter_book に登録された局面・候補手のみに削ぎ落とす

bookutil sort <import_book_filename> <export_book_filename>

bookutil from_apery <import_aperybook_filename> <export_book_filename> [(options)...]
  [unregdepth <value>] : 未登録局面でも探索を続行する深さ
    先手側・後手側いずれかの手順のみ登録された手筋の定跡を抽出するためのもの
    (default: 1)
  [opening <quoted_string>] : 開始局面・手順
    sfen形式、引用符""で囲む、複数指定時は引用符中でカンマ","区切り
    (default: "")
```