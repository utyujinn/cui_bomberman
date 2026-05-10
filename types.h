#ifndef TYPES_H
#define TYPES_H
#define FIELD_X 9
#define FIELD_Y 9
#define BLOCK_X 8
#define BLOCK_Y 8
#define FRAMEWIDTH 8

#define TITLE_WIDTH 72
#define TITLE_HEIGHT 72
#define TITLE_OFFSET_X 8
#define TITLE_OFFSET_Y 8
#define GAME_TIME 3

#define RESULT_WIDTH 72
#define RESULT_HEIGHT 72
#define RESULT_OFFSET_X 8
#define RESULT_OFFSET_Y 8

#define MAX_SPRITES_NUM 50
#define BOMB_TIMER 100
#define BOT_SPEED 3

typedef enum direction{
    FORWARD=0,
    RIGHT=1,
    DOWN=2,
    LEFT=3,
    UP=4
} Direction;

typedef enum spriteKind{
    NONE=0,
    BOMB=1,
    FIRE=2,
    ITEM_A=3,
    ITEM_B=4,
    ITEM_C=5
} SpriteKind;

typedef enum name{
    IRIE = 0,
    KORI = 1,
    MISAKI = 2,
    TORU = 3
} Name;

typedef struct point{//座標用Point型
    char x;
    char y;
    char subx;
    char suby;
} Point;

typedef struct player{//座標に向きも加味した型
    Name name;
    Point pos;
    Direction hdg;//正面:0 右:1 下:2 左:3 上:4
    char status;
    char skillStatus;//スキル用になんか置き場が欲しかったら使う　現状IRIEで特殊爆弾の設置数を管理するのに使ってる　あとToruの振り下ろしタイマー
    char skillTimer;
    char bombStock;//デフォルトで2個
    char isAlive;
    int winCount;
} Player;
/*
Player型仕様まとめ
 - status :
    - ビット0~1 : アイテムA所持フラグ （0 : ノーマル、1 : 1段階加速、2 : 2段階加速）
    - ビット2~4 : 火の長さ（デフォルトで2、アイテム取得で3など増える）
    - ビット5~6 : スキル使用可能回数
    ※アイテムBはbombstockに直接作用
*/

typedef struct cell{//場所ごとに置かれているものを記録する
    char kind;
    char status;
} Cell;
/*
Cell型仕様まとめ
 - kind = 0 : ブランクのマス（自由に動ける）
    - status：未使用
 - kind = 1 : 壁（壊せない　移動できない）
    - status：未使用
 - kind = 2 : 石（壊せる　壊さないと移動できない）
    - status : 入っているアイテム
        - 0 : 空
        - 3 : アイテムAが入っている　※1~2が未使用なのはスプライトの番号と揃えるため
        - 4 : アイテムBが入っている
        - 5 : アイテムCが入っている
*/

typedef struct sprite {//アイテム情報
    Point pos;
    char kind;
    char status;
    char timer;
    char owner;
} Sprite;
/*
Sprite型仕様まとめ
 - kind = 0 : 何もない（明示的には使っていないが予約）
 - kind = 1 : 爆弾
    - status：起爆までのカウントダウン　0になったら起爆
    - 第0ビットはPlayer1が置いた直後に立つ。1ビットはPlayer2が置いた直後に立つ。これで置いた直後に自分が爆弾内を動けるようにする。
        - 改定で起爆までのカウントダウンはtimerになったのかな？
    - owner : player1だったら1、player2だったら2　システム的に設置された場合などは0
 - kind = 2 : 炎
    - status : 向き・種類（最下位ビットが中心用　第2ビットが上下or左右　第3ビットが太or細）
        - 0 : （未使用）
        - 1 : 中心用
        - 2 : 太　左右用
        - 3 : 太　上下用
        - 4 : 細　左右用
        - 5 : 細　上下用
        - 第4ビット～第8ビットは消えるまでの時間　2^5=32あるから最大1秒ちょっと残留出来て十分だと思う　とりあえずデフォでは8フレーム（0b01000xxx）にしておく
    - owner : player1だったら1、player2だったら2　システム的に設置された場合などは0（自分の弾では死なないをやりたいときに使う？）
 - kind = 3 : アイテムA
    - statusはアイテムが消えるまでの時間とかに使える？
 - kind = 4 : アイテムB
 - kind = 5 : アイテムC
*/

typedef enum stage{
    MINE = 0,
    MANSION = 1,
    MINE2 = 2,
    MANSION2 = 3
} Stage;

typedef struct config{
    Stage stage;//こっちは0か1しか使わない
    char stoneRatio;
    char itemARatio;
    char itemBRatio;
    char itemCRatio;
} Config;


typedef struct world{
    Player player1;
    Player player2;
    char chosenName1;
    char chosenName2;
    Stage chosenStage1;
    Stage chosenStage2;
    Config config;

    Cell field[FIELD_Y][FIELD_X];//背景用
    Sprite sprites[MAX_SPRITES_NUM];//アイテム用
} World;

typedef enum gameState{
    STATE_TITLE,
    STATE_HELP,
    STATE_MENU,
    STATE_PLAY,
    STATE_RESULT
} GameState;

/* 
シーン全体を管理する構造体
timer->グローバルタイマー。常に上昇を続ける
timer2->その時々に利用できるタイマー(ex.画面遷移時のカウントダウン)
flag->汎用フラグ
*/
typedef struct scene{
    GameState state;
    World world;
    char input_player1;
    char input_player2;
    int timer;
    int timer2;
    int timer3;
    char flag;
    char gamemode;
    char connection_failed;  // 接続失敗フラグ
} Scene;

#endif
