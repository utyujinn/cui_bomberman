#include "types.h"
#include "graphics.h"
#include "texture.h"


int (*typeToFieldTexture(World* world, char type))[BLOCK_X]{//数字で指定されるブロックの種類をテクスチャのポインタに変換するだけ
    switch(type){
        case 0:
            return itemA;
            break;
        case 1:
            return wall[world->config.stage];
            break;
        case 2:
            return stone[world->config.stage];
            break;
    }
    return itemA;
}

int (*typeToItemTexture(char type))[BLOCK_X]{
    switch(type){
        case 3:
            return itemA;
            break;
        case 4:
            return itemB;
            break;
        case 5:
            return itemC;
            break;
    }
}

void drawBgBuff(int texture[BLOCK_Y][BLOCK_X], int posY, int posX){
    for(int y=0;y<BLOCK_Y;y++){
        for(int x=0;x<BLOCK_X;x++){
            screenBuff[posY * BLOCK_Y + y + FRAMEWIDTH][posX * BLOCK_X + x + FRAMEWIDTH] = texture[y][x];
        }
    }
}

void drawFloorBuff(World* world, int posY, int posX){
    if(posY==0){
        if(posX%2==0){
            drawBgBuff(floor_texture[world->config.stage][0], posY, posX);
        }else {
            drawBgBuff(floor_texture[world->config.stage][1], posY, posX);
        }
    }else if(posY!=0&&posY!=FIELD_Y-1){
        drawBgBuff(floor_texture[world->config.stage][2], posY, posX);
    }else if(posY==FIELD_Y-1){
        if(posX%2==0){
            drawBgBuff(floor_texture[world->config.stage][3], posY, posX);
        }else {
            drawBgBuff(floor_texture[world->config.stage][4], posY, posX);
        }
    }
}

void drawFieldBuff(World *world){
    for(int i = 0; i < FIELD_Y; i ++){
        for(int j = 0; j < FIELD_X; j ++){
            if(world->field[i][j].kind == 0){
                drawFloorBuff(world, i, j);
            }else {
                drawBgBuff(typeToFieldTexture(world, world->field[i][j].kind), i, j);
            }
        }
    }
}

void drawSpriteBuff(int texture[BLOCK_Y][BLOCK_X], Sprite sprite){//指定されたテクスチャをsprite型の位置にscreenBuffに書き込む関数
    for(int y=0;y<BLOCK_Y;y++){
        for(int x=0;x<BLOCK_X;x++){
            if(!(texture[y][x] & 0x01000000)){
                screenBuff[sprite.pos.y * BLOCK_Y + y + FRAMEWIDTH][sprite.pos.x * BLOCK_X + x + FRAMEWIDTH] = texture[y][x];
            }
        }
    }
}

void drawBombBuff(Sprite sprite){
    int idx = 9 - sprite.timer/10;
    if(idx < 0){
        idx = 0;
    }
    drawSpriteBuff(bomb[idx], sprite);
}

void drawItemBuff(World *world){
    for(int i=0;i<MAX_SPRITES_NUM;i++){
        Sprite sprite = world->sprites[i];
        if(sprite.kind != 0){
            if(sprite.kind == 1){
                drawBombBuff(sprite);//爆弾はインデックスとの対応が複雑なので別関数で扱う
            } else if(sprite.kind == 2){//fireの場合
                drawSpriteBuff(fire[(sprite.status & 0b00000111)], sprite);            
            } else {//itemの場合　elseで扱っているのでもっと別のスプライトを追加したときには注意！
                drawSpriteBuff(typeToItemTexture(sprite.kind), sprite);
            }
        }
    }
}

void drawPlayerBuff(World *world){
    //player1
    char player1Diff = world->player1.hdg;
    char player2Diff = world->player2.hdg;
    if(world->player1.skillTimer != 0){//スキル発動時にはテクスチャの後半部分を使う
        if(!(world->player1.skillTimer < 25 && world->player1.skillTimer % 2 == 0)){
            player1Diff += 4;
        }
    }
    if(world->player2.skillTimer != 0){
        if(!(world->player2.skillTimer < 25 && world->player2.skillTimer % 2 == 0)){
            player2Diff += 4;
        }
    }
    if(world->player1.name == TORU && world->player1.skillStatus != 0){
        player1Diff += 5;
    }
    if(world->player1.name == TORU && world->player2.skillStatus != 0){
        player2Diff += 5;
    }
    if(world->player1.isAlive == 0){
        player1Diff = 9;
    }
    if(world->player2.isAlive == 0){
        player2Diff = 9;
    }
    for(int y=0;y<BLOCK_Y;y++){
        for(int x=0;x<BLOCK_X;x++){
            if(!(player[world->player1.name][player1Diff][y][x] & 0x01000000)){
                screenBuff[world->player1.pos.y * BLOCK_Y + world->player1.pos.suby + y + FRAMEWIDTH][world->player1.pos.x * BLOCK_X + world->player1.pos.subx + x + FRAMEWIDTH] = player[world->player1.name][player1Diff][y][x];
            }
        }
    }
    //player2
    for(int y=0;y<BLOCK_Y;y++){
        for(int x=0;x<BLOCK_X;x++){
            if(!(player[world->player2.name][player2Diff][y][x] & 0x01000000)){
                screenBuff[world->player2.pos.y * BLOCK_Y + world->player2.pos.suby + y + FRAMEWIDTH][world->player2.pos.x * BLOCK_X + world->player2.pos.subx + x + FRAMEWIDTH] = player[world->player2.name][player2Diff][y][x];
            }
        }
    }
}


void drawTimerBuff(Scene *scene){
    int time = (30*60*GAME_TIME - scene->timer2)/30;
    char min = time/60;
    char sec = time%60;
    char secl = sec%10;
    char secu = sec/10;
    drawNumberBuff(min,2,36);
    drawNumberBuff(10,2,40);
    drawNumberBuff(secu,2,44);
    drawNumberBuff(secl,2,48);
}

void drawGameBuff(Scene *scene){
    drawFieldBuff(&scene->world);//それぞれのバッファー更新
    drawItemBuff(&scene->world);
    drawPlayerBuff(&scene->world);
    drawFrameBuff(scene);
    drawTimerBuff(scene);
}