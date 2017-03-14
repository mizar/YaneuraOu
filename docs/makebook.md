# 定跡作成(makebook)拡張コマンドまとめ (2017Early-)

[./解説.txt](./解説.txt) もあわせて参照のこと。

## sfen棋譜をそのままなぞる定跡を作成

`makebook from_sfen` コマンドの解説

- TODO: サンプルor解説を書く

## sfen棋譜の局面をそれぞれ思考した定跡を作成

`makebook think` コマンドの解説

- TODO: サンプルor解説を書く

## 定跡ファイルを棋譜ファイルに書き出し

`makebook {to_sfen | to_kif1 | to_kif2 | to_csa1}` コマンドの解説

- TODO: サンプルor解説を書く

## 定跡ファイルの自己生成

`makebook to_sfen` コマンドと `makebook think` コマンドを交互に使い、定跡ファイルを自己生成する方法の紹介

`opening` オプションによる開始局面・手順の指定方法の解説

- TODO: サンプルor解説を書く

## 定跡ファイルの合成（マージ）

`makebook merge` コマンドの解説

- TODO: サンプルor解説を書く

## 外部で生成されたやねうら棋譜定跡ファイルの取り込み

- sfen文字列（の持ち駒表記）が正規化されていない
  - [sfen文字列は一意に定まらない件](http://yaneuraou.yaneu.com/2016/07/15/sfen%E6%96%87%E5%AD%97%E5%88%97%E3%81%AF%E4%B8%80%E6%84%8F%E3%81%AB%E5%AE%9A%E3%81%BE%E3%82%89%E3%81%AA%E3%81%84%E4%BB%B6/)
  - [sfen文字列は本来は一意に定まる件](http://yaneuraou.yaneu.com/2016/07/15/sfen%E6%96%87%E5%AD%97%E5%88%97%E3%81%AF%E6%9C%AC%E6%9D%A5%E3%81%AF%E4%B8%80%E6%84%8F%E3%81%AB%E5%AE%9A%E3%81%BE%E3%82%8B%E4%BB%B6/)
- sfen文字列による辞書順に局面が並んでいない
  - 辞書順に並んでいないと BookOnTheFly の機能が使えない
- 手数のみ異なる同一局面が複数含まれる

このような定跡ファイルを正規化したい場合に、 `makebook sort` コマンドを使う。

2017Early vX.XXx 以降のバージョンでは、`makebook {from_sfen | think | merge}` コマンドの出力は辞書順に出力されるため、必ずしも `makebook sort` コマンドによる後処理は必要ない。（sfen文字列の持ち駒表記が正規化されていない定跡ファイルを扱った場合を除く）

## makebook コマンドリファレンス

```
makebook from_sfen <sfen_filename> <book_filename> [moves <value>]
makebook think <sfen_name> <book_name> [(options)...]
options:
  [moves <value>] : 最大手数(default: 16)
  [depth <value>] : 探索深さ(default: 24)
  [startmoves <value>] : 開始手数(default: 1)
  [cluster <id> <num>] : クラスタ用間引き指定(default: 1 1)
  [progresstype <value>] : 思考中経過表示のタイプ(default: 0)
    0:標準 1:冗長
  [blackonly] : white手番の局面を思考対象から省く
  [whiteonly] : black手番の局面を思考対象から省く

makebook {to_sfen | to_kif1 | to_kif2 | to_csa1} <book_filename> <sfen_filename> [(options)...]
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

makebook merge <import_book0_name> <import_book1_name> <export_book_name>

makebook sort <import_book_name> <export_book_name>
```