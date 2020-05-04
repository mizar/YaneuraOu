#ifndef YANEURAOU_PARAM_H_INCLUDED
#define YANEURAOU_PARAM_H_INCLUDED

// パラメーターの説明に "fixed"と書いてあるパラメーターはランダムパラメーター化するときでも変化しない。
// 「前提depth」は、これ以上ならその枝刈りを適用する(かも)の意味。
// 「適用depth」は、これ以下ならその枝刈りを適用する(かも)の意味。

// 現在の値から、min～maxの範囲で、+step,-step,+0を試す。
// interval = 2だと、-2*step,-step,+0,+step,2*stepの5つを試す。

//
// futility pruning
//

// 深さに比例したfutility pruning
// depth手先で評価値が変動する幅が = depth * (PARAM_FUTILITY_MARGIN_ALPHA1 - improving*PARAM_FUTILITY_MARGIN_ALPHA2)
// 元の値 = 217
// [PARAM] min:100,max:240,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA1 = 217;

// 元の値 = 50
// [PARAM] min:25,max:100,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA2 = 50;


// 

// 元の値 = 172
// [PARAM] min:100,max:240,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_BETA = 172;


// 静止探索でのfutility pruning
// 元の値 = 154
// [PARAM] min:50,max:160,step:5,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_QUIET = 154;

// futility pruningの適用depth。
// この制限自体が要らない可能性がある。→　そうでもなかった。
// 元の値 = 6
// [PARAM] min:5,max:15,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_RETURN_DEPTH = 6;

// 親nodeでのfutilityの適用depth。
// この枝刈り、depthの制限自体が要らないような気がする。→　そうでもなかった。
// 元の値 = 6
// [PARAM] min:5,max:20,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_DEPTH = 6;

// 親nodeでのfutility margin
// 元の値 = 235
// [PARAM] min:100,max:300,step:5,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1 = 235;

// staticEvalから減算するmargin
// 元の値 = 200
// [PARAM] min:0,max:300,step:25,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2 = 248;

// depthが2乗されるので影響大きい
// 元の値 = 32
// [PARAM] min:20,max:50,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1 = 32;

// depthが2乗されるので影響大きい
// 元の値 = 35
// [PARAM] min:20,max:60,step:3,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2 = 51;

//
// null move dynamic pruning
//

// null move dynamic pruningのときの
//  Reduction = (α + β * depth ) / 256 + ...みたいなαとβ

// 元の値 = 854
// [PARAM] min:500,max:1500,step:5,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_ALPHA = 854;

// 元の値 = 68
// [PARAM] min:50,max:100,step:8,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_BETA = 68;

// 元の値 = 32
// [PARAM] min:10,max:60,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_MARGIN = 32;

// null moveでbeta値を上回ったときに、これ以下ならreturnするdepth。適用depth。
// 元の値 = 13
// [PARAM] min:4,max:16,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_RETURN_DEPTH = 13;


//
// ProbCut
//

// probcutの前提depth
// 元の値 = 5
// [PARAM] min:3,max:10,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PROBCUT_DEPTH = 5;

// probcutのmargin
//    式 = beta + PARAM_PROBCUT_MARGIN1 - improving * PARAM_PROBCUT_MARGIN2
//   improvingの効果怪しいので抑え気味にしておく。
// 元の値 = 216
// [PARAM] min:100,max:300,step:3,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PROBCUT_MARGIN1 = 189;

// 元の値 = 48
// [PARAM] min:20,max:80,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PROBCUT_MARGIN2 = 45;

//
// singular extension
//

// singular extensionの前提depth。
// これ変更すると他のパラメーターががらっと変わるので固定しておく。
// 10秒設定だと6か8あたりに局所解があるようだ。
// 元の値 = 6
// [PARAM] min:4,max:13,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_EXTENSION_DEPTH = 6;

// singular extensionのmarginを計算するときの係数
// rBeta = std::max(ttValue - PARAM_SINGULAR_MARGIN * depth / (8 * ONE_PLY), -VALUE_MATE);
// 元の値 = 128
// [PARAM] min:128,max:400,step:4,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_MARGIN = 128;

// singular extensionで浅い探索をするときの深さに関する係数
// このパラメーター、長い時間でないと調整できないし下手に調整すべきではない。
// 元の値 = 16
// [PARAM] min:8,max:32,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_SEARCH_DEPTH_ALPHA = 16;


//
// pruning by move count,history,etc..
//

// move countによる枝刈りをする深さ。適用depth。
// 元の値 = 16
// [PARAM] min:8,max:32,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PRUNING_BY_MOVE_COUNT_DEPTH = 16;

// historyによる枝刈りをする深さ。適用depth。
// これ、将棋ではそこそこ上げたほうが長い時間では良さげ。
// 元の値 = 4
// [PARAM] min:2,max:32,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PRUNING_BY_HISTORY_DEPTH = 4;


// historyの値によってreductionするときの係数
// これ、元のが (hist - 8000) / 20000みたいな意味ありげな値なので下手に変更しないほうが良さげ。
// 元の値 = 4926
// [PARAM] min:2000,max:8000,step:100,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_REDUCTION_BY_HISTORY = 4926;


//
// razoring pruning
// 

// 元の値 = 531
// [PARAM] min:400,max:700,step:5,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_RAZORING_MARGIN = 531;


//
// LMR reduction table
//

// 元の値 = 131
// [PARAM] min:64,max:256,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_REDUCTION_ALPHA = 135;


//
// etc..
// 

// この個数までquietの指し手を登録してhistoryなどを増減させる。
// 元の値 = 64
// 将棋では駒打ちがあるから少し増やしたほうがいいかも。
// →　そうでもなかった。固定しておく。
// historyの計算でこの64に基づいた値を使っている箇所があるからのような気がする。よく考える。
// [PARAM] min:32,max:128,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_QUIET_SEARCH_COUNT = 64;


// 静止探索での1手詰め
// 元の値 = 1
// →　1スレ2秒で対技巧だと有りのほうが強かったので固定しておく。
// [PARAM] min:0,max:1,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_QSEARCH_MATE1 = 1;

// 通常探索での1手詰め
// →　よくわからないが1スレ2秒で対技巧だと無しのほうが強かった。
//     1スレ3秒にすると有りのほうが強かった。やはり有りのほうが良いのでは..
// 元の値 = 1
// [PARAM] min:0,max:1,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SEARCH_MATE1 = 1;

// 1手詰めではなくN手詰めを用いる
// 元の値 = 1
// [PARAM] min:1,max:5,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_WEAK_MATE_PLY = 1;


// aspiration searchの増加量
// 元の値 = 21
// [PARAM] min:12,max:40,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_ASPIRATION_SEARCH_DELTA = 21;


// 評価関数での手番の価値
// 元の値 = 20
// [PARAM] min:10,max:50,step:5,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_EVAL_TEMPO = 20;


#endif
