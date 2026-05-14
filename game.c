#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "types.h"

int d[5]={0,1,0,-1,0};//方向ベクトル配列（右、下、左、上）

void init_world(World *world){
    srand((unsigned int)time(NULL));
    if(rand() < RAND_MAX / 2){
        world->config.stage = world->chosenStage1;
    }else{
        world->config.stage = world->chosenStage2;
    }
    if(world->config.stage & 0b00000010){//2が付くステージ名の場合
        world->config.stage -= 2;//ステージ番号を0か1に揃える
        world->config.stoneRatio = 4;//stoneRatioは小さくすると確率が下がる
        world->config.itemARatio = 15;//itemXRatioは0~itemA, itemA~itemB, itemB~itemCの部分が出現確立にパーセントで対応
        world->config.itemBRatio = 30;
        world->config.itemCRatio = 45;
    }else{
        world->config.stoneRatio = 2;
        world->config.itemARatio = 5;
        world->config.itemBRatio = 10;
        world->config.itemCRatio = 15;
    }

    world->player1 = (Player){.pos = {0, 0, 0, 0}, .hdg = 0, .isAlive = 1, .bombStock = 2, .status = 0b01001000, .skillStatus = 0, .name = world->chosenName1, .winCount = world->player1.winCount};
    world->player2 = (Player){.pos = {FIELD_X - 1, FIELD_Y - 1, 0, 0}, .hdg = 0, .isAlive = 1, .bombStock = 2, .status = 0b01001000, .skillStatus = 0, .name = world->chosenName2, .winCount = world->player2.winCount};
    for(int i = 0; i < FIELD_Y; i ++){
        for(int j = 0; j < FIELD_X; j ++){
            if(i % 2 && j % 2){
                world->field[i][j].kind = 1;
                world->field[i][j].status = 0;
            }else{
                if(rand() > RAND_MAX / world->config.stoneRatio){
                    world->field[i][j].kind = 2;
                    int random = rand();
                    if(random < RAND_MAX / (100 / world->config.itemARatio)){
                        world->field[i][j].status = 3;//1/10ずつの確率でアイテムA~Cが入っている
                    }else if(random < RAND_MAX / (100 / world->config.itemBRatio)){
                        world->field[i][j].status = 4;
                    }else if(random < RAND_MAX / (100 / world->config.itemCRatio)){
                        world->field[i][j].status = 5;
                    }else{
                        world->field[i][j].status = 0;
                    }
                }else{
                    world->field[i][j] = (Cell){.kind = 0, .status = 0};
                }
            }  
        }
    }
    for(int i = 0; i <= 2; i ++){//詰み回避でスタート地点近辺は確実に空白にする
        world->field[i][0] = (Cell){.kind = 0, .status = 0};
        world->field[0][i] = (Cell){.kind = 0, .status = 0};
        world->field[FIELD_Y - 1 - i][FIELD_X - 1] = (Cell){.kind = 0, .status = 0};
        world->field[FIELD_Y - 1][FIELD_X - 1 - i] = (Cell){.kind = 0, .status = 0};
    }
    for(int i=0;i<MAX_SPRITES_NUM;i++){
        world->sprites[i].kind=0;
        world->sprites[i].status=0;
        world->sprites[i].pos.x=0;
        world->sprites[i].pos.y=0;
    }
}

Player* player_pointer(World *world, char player){//これ何気にこれまで使ってなかったけど大事じゃね？
    if(player == 1){
        return &(world->player1);
    }else if(player == 2){
        return &(world->player2);
    }
}


int iswall(World *world, Point *p){
    if(world->field[p->y][p->x].kind==1||world->field[p->y][p->x].kind==2)return 1;
    if(p->subx!=0&&p->suby!=0){
        if(world->field[p->y+1][p->x+1].kind==1||world->field[p->y+1][p->x+1].kind==2)return 1;
    }
    if(p->subx!=0){
        if(world->field[p->y][p->x+1].kind==1||world->field[p->y][p->x+1].kind==2)return 1;
    }
    if(p->suby!=0){
        if(world->field[p->y+1][p->x].kind==1||world->field[p->y+1][p->x].kind==2)return 1;
    }
    return 0;
}

// サブピクセル単位での座標を取得する関数
char get_abs_sub_x(Point *pos){
    return pos->x*BLOCK_X + pos->subx;
}
char get_abs_sub_y(Point *pos){
    return pos->y*BLOCK_Y + pos->suby;
}

// Point構造体の座標をdx,dy分移動した新しいPoint構造体を返す関数
Point addPoint(Point *a, int dx, int dy){
    Point res;
    char abs_x = get_abs_sub_x(a);
    char abs_y = get_abs_sub_y(a);
    abs_x += dx;
    abs_y += dy;
    if(abs_x < 0) abs_x = 0;
    if(abs_y < 0) abs_y = 0;
    if(abs_x >= (FIELD_X-1) * BLOCK_X + 1) abs_x = (FIELD_X-1) * BLOCK_X;
    if(abs_y >= (FIELD_Y-1) * BLOCK_Y + 1) abs_y = (FIELD_Y-1) * BLOCK_Y;
    res.x = abs_x / BLOCK_X;
    res.subx = abs_x % BLOCK_X;
    res.y = abs_y / BLOCK_Y;
    res.suby = abs_y % BLOCK_Y;
    return res;
}

// 2つのPoint構造体が少しでも重なっているかを判定する関数
char isOverlap(Point *a, Point *b){
    char ax1 = get_abs_sub_x(a);
    char ay1 = get_abs_sub_y(a);
    char bx1 = get_abs_sub_x(b);
    char by1 = get_abs_sub_y(b);
    if(abs(ax1 - bx1) >= BLOCK_X) return 0;
    if(abs(ay1 - by1) >= BLOCK_Y) return 0;
    return 1;
}

// プレイヤーが爆弾と重なっているかを判定する関数(通過フラグも考慮、置いた直後は通過可能)
char stack_with_bomb(World *world, Point newPos, char player){
    for(int i=0;i<MAX_SPRITES_NUM;i++){
        if(world->sprites[i].kind==BOMB){
            if(isOverlap(&newPos, &world->sprites[i].pos)){
                // プレイヤーごとの通過フラグをチェック
                if(player == 1){
                    if((world->sprites[i].status & 0b00000001) == 0){
                        return 1; // Player1の通過フラグが立っていない場合は衝突
                    }
                }else if(player == 2){
                    if((world->sprites[i].status & 0b00000010) == 0){
                        return 1; // Player2の通過フラグが立っていない場合は衝突
                    }
                }
            }
        }
    }
    return 0;
}

char isWallThrough(World *world, char player){//KORI用の壁抜けを判定する関数　壁抜け有効なら1を返す
    if(player == 1){
        if(world->player1.name == KORI && world->player1.skillTimer != 0){
            return 1;
        }else{
            return 0;
        }
    }else if(player == 2){
        if(world->player2.name == KORI && world->player2.skillTimer != 0){
            return 1;
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}

char confirm_movement(World *world, Point newPos, char player){//移動が成功すれば0を返す、ぶつかっていたら1を返す
    if(isWallThrough(world, player)){
        if(player == 1){
            world->player1.pos = newPos;
        }else if(player == 2){
            world->player2.pos = newPos;
        }
        return 0;
    }else{
        if(!iswall(world, &newPos)){
            if(!stack_with_bomb(world, newPos, player)){
                if(world->field[newPos.y][newPos.x].kind == 0){
                    if(player == 1){
                        world->player1.pos = newPos;
                    }else if(player == 2){
                        world->player2.pos = newPos;
                    }
                }
            }
            return 0;
        }else{
            return 1;
        }
    }    
}

Point playerPos(World *world, char player){
    if(player == 1){
        return world->player1.pos;
    }else if(player == 2){
        return world->player2.pos;
    }
}


// プレイヤーを移動させる補助関数
void sub_move_player(World *world,Direction dir,char count,char player){
    while(count--){
        Point newPos;
        if(player == 1){
            newPos = world->player1.pos;
        }else if(player == 2){
            newPos = world->player2.pos;
        }

        if(dir==RIGHT){
            newPos = addPoint(&newPos, 2, 0);
        }else if(dir==DOWN){
            newPos = addPoint(&newPos, 0, 2);
        }else if(dir==LEFT){
            newPos = addPoint(&newPos, -2, 0);
        }else if(dir==UP){
            newPos = addPoint(&newPos, 0, -2);
        }
        // 壁や爆弾と衝突しなければ移動を確定
        if(confirm_movement(world, newPos, player)){
            Point currentPos;
            if(player == 1){
                currentPos = world->player1.pos;
            }else if(player == 2){
                currentPos = world->player2.pos;
            }

            if(dir == UP){
                if(playerPos(world, player).subx == 2){
                    newPos = addPoint(&currentPos, -2, -2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).subx == 6){
                    newPos = addPoint(&currentPos, 2, -2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).subx == 4){
                    newPos = addPoint(&currentPos, -4, -2);
                    if(confirm_movement(world, newPos, player)){
                        newPos = addPoint(&currentPos, 4, -2);
                        confirm_movement(world, newPos, player);
                    }
                }
            }else if(dir == DOWN){
                if(playerPos(world, player).subx == 2){
                    newPos = addPoint(&currentPos, -2, 2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).subx == 6){
                    newPos = addPoint(&currentPos, 2, 2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).subx == 4){
                    newPos = addPoint(&currentPos, -4, 2);
                    if(confirm_movement(world, newPos, player)){
                        newPos = addPoint(&currentPos, 4, 2);
                        confirm_movement(world, newPos, player);
                    }
                }
            }else if(dir == LEFT){
                if(playerPos(world, player).suby == 2){
                    newPos = addPoint(&currentPos, -2, -2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).suby == 6){
                    newPos = addPoint(&currentPos, -2, 2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).suby == 4){
                    newPos = addPoint(&currentPos, -2, -4);
                    if(confirm_movement(world, newPos, player)){
                        newPos = addPoint(&currentPos, -2, 4);
                        confirm_movement(world, newPos, player);
                    }
                }
            }else if(dir == RIGHT){
                if(playerPos(world, player).suby == 2){
                    newPos = addPoint(&currentPos, 2, -2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).suby == 6){
                    newPos = addPoint(&currentPos, 2, 2);
                    confirm_movement(world, newPos, player);
                }else if(playerPos(world, player).suby == 4){
                    newPos = addPoint(&currentPos, 2, -4);
                    if(confirm_movement(world, newPos, player)){
                        newPos = addPoint(&currentPos, 2, 4);
                        confirm_movement(world, newPos, player);
                    }
                }
            }
        }
    }
}

void move_player(World *world,Direction dir,char player){
    if(player==1){
        world->player1.hdg=dir;
        if((world->player1.status & 0b00000011)==2){
            sub_move_player(world,dir,4,player);
        }else if((world->player1.status & 0b00000011)==1){
            sub_move_player(world,dir,2,player);
        }else{
            sub_move_player(world,dir,1,player);
        }
    }else if(player==2){
        world->player2.hdg=dir;
        if((world->player2.status & 0b00000011)==2){
            sub_move_player(world,dir,4,player);
        }else if((world->player2.status & 0b00000011)==1){
            sub_move_player(world,dir,2,player);
        }else{
            sub_move_player(world,dir,1,player);
        }
    }
}


char find_spriteIndex(World *world){//空いているスプライトの番号を返す
    for(int i = 0; i < MAX_SPRITES_NUM; i ++){
        if(world->sprites[i].kind == 0){
            return i;
            break;
        }
    }
}


void place_bomb(World *world, char player){
    Player* PlayerPointer = player_pointer(world, player);
    if(PlayerPointer->bombStock > 0){
        PlayerPointer->bombStock --;
        char i = find_spriteIndex(world);
        world->sprites[i].kind=BOMB;
        world->sprites[i].timer=BOMB_TIMER;

        world->sprites[i].owner = player;
        world->sprites[i].status=player; // 通過フラグを立てる
        if(PlayerPointer->pos.subx >= 4){
            world->sprites[i].pos.x=PlayerPointer->pos.x + 1;
        }else{
            world->sprites[i].pos.x=PlayerPointer->pos.x;
        }
        if(PlayerPointer->pos.suby >= 4){
            world->sprites[i].pos.y=PlayerPointer->pos.y + 1;
        }else{
            world->sprites[i].pos.y=PlayerPointer->pos.y;
        }
        world->sprites[i].pos.subx=0;
        world->sprites[i].pos.suby=0;
    }
}

void place_bomb_custom(World *world, Point pos, char timer, char owner){
    char i = find_spriteIndex(world);
    world->sprites[i].kind=BOMB;
    world->sprites[i].owner=owner;//1, 2以外を指定すると爆発範囲が1になる　IRIEの無敵判定にも使う
    world->sprites[i].timer=timer;
    world->sprites[i].pos.x=pos.x;
    world->sprites[i].pos.y=pos.y;
    world->sprites[i].pos.subx=0;
    world->sprites[i].pos.suby=0;
}

void skill_MISAKI(World *world){
    for(int i = 0; i < 5; i ++){
        Point pos = {.subx = 0, .suby = 0};
        pos.x = rand() / (RAND_MAX / FIELD_X);
        pos.y = rand() / (RAND_MAX / FIELD_Y);
        place_bomb_custom(world, pos, 25, 100);
    }
}

void activate_skill(World *world, char player){
    Player* PlayerPointer = player_pointer(world, player);
    if((PlayerPointer->status & 0b01100000) != 0){//スキル使用回数が残っている場合
        PlayerPointer->status -= 0b00100000;
        if(PlayerPointer->name != MISAKI){
            PlayerPointer->skillTimer = 100;
        }else{//MISAKIの場合即時発効なのでタイマは使用しない
            skill_MISAKI(world);
        }
    }
}

// 2つのPoint構造体がほぼ同じ位置にあるかを判定する関数(1/2重なっていればOK)
char isSamePos(Point a, Point b){
    //全体の座標をサブピクセル単位で比較
    char ax = get_abs_sub_x(&a);
    char ay = get_abs_sub_y(&a);
    char bx = get_abs_sub_x(&b);
    char by = get_abs_sub_y(&b);
    if(abs(ax-bx)<=4 && abs(ay-by)<=4){
        return 1;
    }else{
        return 0;
    }
}

char isFireExist(World *world, Point a){//爆弾の位置aに火が重なっているか判定する
    char status = 0;
    for(int i = 0; i < MAX_SPRITES_NUM; i ++){
        if(world->sprites[i].kind == 2){//火である
            if(isSamePos(world->sprites[i].pos, a)){//火と重なっている
                status = 1;
                break;
            }
        }
    }
    return status;
}

void process_bomb(Sprite *sprite, World *world){
    // 通過フラグのクリア処理
    if(sprite->status & 0b00000001){
        if(!isOverlap(&world->player1.pos, &sprite->pos)){
            sprite->status &= ~0b00000001; // Player1の通過フラグをクリア
        }
    }
    if(sprite->status & 0b00000010){
        if(!isOverlap(&world->player2.pos, &sprite->pos)){
            sprite->status &= ~0b00000010; // Player2の通過フラグをクリア
        }
    }

    // カウントダウン（下位6ビットのみ）
    sprite->timer--;

    if(sprite->timer == 0 || isFireExist(world, sprite->pos)){
        //爆発処理
        char bx=sprite->pos.x;
        char by=sprite->pos.y;
        //中心
        world->field[by][bx]=(Cell){.kind=0,.status=0};//背景の扱いは空白に
        world->sprites[find_spriteIndex(world)] = (Sprite){.kind = 2, .status = 1 | 0b10000000, .pos.x = bx, .pos.y = by, .owner = sprite->owner};//火のスプライトを追加
        
        //周囲4方向
        char length = 1;
        if(sprite->owner == 1){
            length = (world->player1.status & 0b00011100) >> 2;
        }else if(sprite->owner == 2){
            length = (world->player2.status & 0b00011100) >> 2;
        }//ほかにシステム上置かれるものならまあとりあえずデフォ値の1が入る
        for(int dir=0;dir<4;dir++){
            for(int dist=1;dist<=length;dist++){
                char distGfx = 1;
                if(dist > length / 2){//火の長さの半分より先はdistGfxを2にする
                    distGfx = 2;
                }
                int nx=bx+d[dir+1]*dist;
                int ny=by+d[dir]*dist;
                if(nx<0||nx>=FIELD_X||ny<0||ny>=FIELD_Y)break;
                if(world->field[ny][nx].kind==1)break;//壁に当たったら終了
                if(world->field[ny][nx].kind==2){//石に当たった場合
                    if(world->field[ny][nx].status != 0){//アイテムが入っている場合
                        world->sprites[find_spriteIndex(world)] = (Sprite){.kind = world->field[ny][nx].status, .status = 0, .pos.x = nx, .pos.y = ny};//アイテムのスプライトを追加
                    }
                    world->field[ny][nx]=(Cell){.kind=0,.status=0};//背景の扱いは空白に
                    world->sprites[find_spriteIndex(world)] = (Sprite){.kind = 2, .status = (abs(d[dir]) + 2 * distGfx) | 0b10000000, .pos.x = nx, .pos.y = ny, .owner = sprite->owner};//火のスプライトを追加
                    break;//石に当たったら終了
                }
                world->field[ny][nx]=(Cell){.kind=0,.status=0};
                world->sprites[find_spriteIndex(world)] = (Sprite){.kind = 2, .status = (abs(d[dir]) + 2 * distGfx) | 0b10000000, .pos.x = nx, .pos.y = ny, .owner = sprite->owner};//火のスプライトを追加
            }
        }
        //爆弾のスプライトを消す
        sprite->kind=0;
        sprite->status=0;
        //当該プレイヤーのbombstockを戻す
        if(sprite->owner == 1){
            world->player1.bombStock ++;
        }else if(sprite->owner == 2){
            world->player2.bombStock ++;
        }
    }
}

void process_fire(Sprite *sprite, World *world){
    sprite->status -= 0b00001000;
    if(sprite->status < 0b00001000){
        *sprite = (Sprite){.kind = 0, .status = 0};
    }
}


void process_sprites(World *world){
    for(int i=0;i<MAX_SPRITES_NUM;i++){
        if(world->sprites[i].kind==BOMB){
            process_bomb(&world->sprites[i], world);//爆弾のカウントダウン
        }else if(world->sprites[i].kind == FIRE){
            process_fire(&world->sprites[i], world);//火のカウントダウン
        }
    }
}

void process_skill(World *world){
    if(world->player1.skillTimer != 0){
        world->player1.skillTimer --;
    }
    if(world->player2.skillTimer != 0){
        world->player2.skillTimer --;
    }
    if(world->player1.name == TORU && world->player1.skillStatus != 0){
        world->player1.skillStatus --;
    }
    if(world->player2.name == TORU && world->player2.skillStatus != 0){
        world->player2.skillStatus --;
    }
}


void skill_IRIE(World *world, char player){
    place_bomb_custom(world, player_pointer(world, player)->pos, 25, (100 + player));//1PのIRIEなら101、2PのIRIEなら102をそれぞれ自分の爆弾として無敵化
}

void skill_TORU(World *world, char player){
    char opponent;
    if(player == 1){
        opponent = 2;
    }else if(player == 2){
        opponent = 1;
    }
    Player* playerPointer = player_pointer(world, player);
    Player* opponentPointer = player_pointer(world, opponent);
    Point playersFront = playerPointer->pos;
    switch(playerPointer->hdg){
        case RIGHT:
            playersFront.x ++;
            break;
        case LEFT:
            playersFront.x --;
            break;
        case DOWN:
            playersFront.y ++;
            break;
        case UP:
            playersFront.y --;
            break;
    }
    playerPointer->skillStatus = 5;

    if(isSamePos(opponentPointer->pos, playerPointer->pos) || isSamePos(opponentPointer->pos, playersFront)){//自分とisSamePos、あるいは自分の1マス先とisSamePos
        opponentPointer->isAlive = 0;
    }
}

void process_input(World *world,char input,char player){
    if(input=='d' || input=='D')move_player(world,RIGHT,player);
    else if(input=='s' || input=='S')move_player(world,DOWN,player);
    else if(input=='a' || input=='A')move_player(world,LEFT,player);
    else if(input=='w' || input=='W')move_player(world,UP,player);
    if((input>='A' && input<='Z') || input==' ')place_bomb(world,player);//スペースキー以外にもShiftでも置ける
    if(input == 'q' || input == 'Q' || input == '\n'){
        if(player_pointer(world, player)->skillTimer == 0){
            activate_skill(world, player);
        }else{
            if(player_pointer(world, player)->name == IRIE){
                skill_IRIE(world, player);
            }else if(player_pointer(world, player)->name == TORU){
                skill_TORU(world, player);
            }
        }
    }
}


void process_sprites_contacts_with_player(World *world){
    for(int i = 0; i < MAX_SPRITES_NUM; i ++){
        if(world->sprites[i].kind == FIRE){//火のスプライト
            if(isSamePos(world->player1.pos, world->sprites[i].pos)){//火のスプライトと座標が一致
                if(!(world->player1.name == IRIE && world->sprites[i].owner == 101)){
                    world->player1.isAlive=0;
                }
            }
            if(isSamePos(world->player2.pos, world->sprites[i].pos)){//火のスプライトと座標が一致
                if(!(world->player2.name == IRIE && world->sprites[i].owner == 102)){
                    world->player2.isAlive=0;
                }
            }
        }
        if(world->sprites[i].kind == ITEM_A){
            if(isSamePos(world->player1.pos, world->sprites[i].pos)){//アイテムのスプライトと座標が一致
                //アイテム取得処理
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};//アイテムスプライトを消す
                if((world->player1.status & 0b00000011)==0){//まだ加速してないとき
                    world->player1.status += 1;
                    //↓0.5マス移動したまま1マス移動しかできなくて別方向の路地に入れなくなることへの対策のつもりなんだけど、どうやったら発生するのかよくわからなくなったので保留
                    //以下の修正をしないなら2つのif文はくっつけられるね
                    /*
                    if(world->player1.hdg & 0b00000001 == 1){//RIGHTかLEFT。横向き
                        if(world->player1.pos.subx % 4 == 2){
                            sub_move_player(world, world->player1.hdg, 2, 1);
                        }
                    }else{
                        if(world->player1.pos.suby % 4 == 2){
                            sub_move_player(world, world->player1.hdg, 2, 1);
                        }
                    }
                    */
                }
                else if((world->player1.status & 0b00000011)==1){//1段階加速済みのとき
                    world->player1.status += 1;
                    /*
                    if(world->player1.hdg & 0b00000001 == 1){//RIGHTかLEFT。横向き
                        if(world->player1.pos.subx % 8 == 4){
                            sub_move_player(world, world->player1.hdg, 4, 1);
                        }
                    }else{
                        if(world->player1.pos.suby % 8 == 4){
                            sub_move_player(world, world->player1.hdg, 4, 1);
                        }
                    }
                    */
                }
            }
            if(isSamePos(world->player2.pos, world->sprites[i].pos)){//アイテムのスプライトと座標が一致
                //アイテム取得処理
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};//アイテムスプライトを消す
                if((world->player2.status & 0b00000011)==0)world->player2.status += 1;
                else if((world->player2.status & 0b00000011)==1)world->player2.status += 1;
            }
        }else if(world->sprites[i].kind == ITEM_B){
            if(isSamePos(world->player1.pos, world->sprites[i].pos)){
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};
                world->player1.bombStock ++;
            }
            if(isSamePos(world->player2.pos, world->sprites[i].pos)){
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};
                world->player2.bombStock ++;
            }
        }else if(world->sprites[i].kind == ITEM_C){
            if(isSamePos(world->player1.pos, world->sprites[i].pos)){
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};
                world->player1.status += 4;//第0・1ビットを除くため4単位で加算する
            }
            if(isSamePos(world->player2.pos, world->sprites[i].pos)){
                world->sprites[i] = (Sprite){.kind = 0, .status = 0};
                world->player2.status += 4;
            }
        }
    }
}

// ゲームプレイ中の1フレーム分の処理を行う関数,死亡したらタイトル画面に戻る
void process_frame(Scene *scene,char input_player1,char input_player2){
    scene->timer++;
    if(!scene->flag){
        scene->timer2++;
        process_sprites(&scene->world);
        process_skill(&scene->world);
        process_input(&scene->world,input_player1,1);
        process_input(&scene->world,input_player2,2);
        process_sprites_contacts_with_player(&scene->world);
        //時間が来たらどちらも殺す
        if(scene->timer2==30*60*GAME_TIME && scene->world.player1.isAlive && scene->world.player2.isAlive){
            scene->world.player1.isAlive = 0;
            scene->world.player2.isAlive = 0;
        }
        if(!scene->world.player1.isAlive || !scene->world.player2.isAlive){
            scene->flag=1;
        }
    }
    else{
        scene->timer3++;
        if(scene->timer3>90){
            scene->flag=0;
            scene->timer3=0;
            scene->state=STATE_RESULT;
        }
    }
}