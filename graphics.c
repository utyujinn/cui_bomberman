#include <stdio.h>
#include "types.h"
#include <unistd.h>
#include "graphics_game.h"
#include "texture.h"

int screenBuff1[BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2][BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = {{0}};//描画バッファー こいつらを交互に使って描画する
int screenBuff2[BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2][BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = {{0}};
int (*screenBuff)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = screenBuff1;
int (*screenBuffPrev)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = screenBuff2;

//hsv:0xahhhssvvとして表す。h->0~360,s->0~255,v->0~255
//色相についてhttps://www.peko-step.com/tool/hsvrgb.html#ppick2

void drawBlack(){
    for(int i = 0; i < BLOCK_Y * FIELD_Y; i ++){
        for(int j = 0; j < BLOCK_X * FIELD_X; j ++){
            screenBuff[i+FRAMEWIDTH][j+FRAMEWIDTH] = 0;
        }
    }
}

void printText2Buff(Scene* scene, const char* text, char x, char y, char width){//yには偶数を指定すること！
    char row = y;
    char col = x;
    for(int i = 0; text[i] != 0; i ++){
        if(text[i] == '\n'){
            col = x - 1;
            row += 2;
        }else{
            screenBuff[row+FRAMEWIDTH][col+FRAMEWIDTH] = 0x80000000 | text[i];//偶数番目のピクセルに0x80000000を付けることで文字表示ができる
        }

        if(col < x + width - 1){
            col ++;
        }else{
            col = x;
            row += 2;
        }
    }
}

int rgbTohsv(int color){
    int alpha=(color&0b01000000)<<4;
    int r=(color&0xFF0000)>>16;
    int g=(color&0x00FF00)>>8;
    int b=(color&0x0000FF);
    int h,s,v;
    int mx=(r>g?(r>b?r:b):(g>b?g:b));
    int mn=(r<g?(r<b?r:b):(g<b?g:b));
    //hの計算
    if(mx==mn){
        h=0;
    }else if(mx==r){
        h = (60*(g-b))/(mx-mn);
    }else if(mx==g){
        h = (60*(b-r))/(mx-mn)+120;
    }else if(mx==b){
        h = (60*(r-g))/(mx-mn)+240;
    }
    if(h==360)h=0;
    if(h<0)h+=360;
    //sの計算
    if(mx==0){
        s=0;
    }else{
        s=((mx-mn)*255)/mx;
    }
    //vの計算
    v=mx;
    return alpha|(h<<16)|(s<<8)|v;
}

int hsvTorgb(int color){
    int alpha = (color & 0b10000000) >> 4;
    int h = (color & 0x0FFF0000) >> 16;
    int s = (color & 0x0000FF00) >> 8;
    int v = (color & 0x000000FF);
    int r, g, b;
    int mx = v;
    int mn = mx - (s * mx) / 255;
    if (h>=0&&h<60){
        r=mx;
        g = (h * (mx-mn)) / 60 + mn;
        b=mn;
    }else if(h>=60&&h<120){
        r = ((120 - h) * (mx - mn)) / 60 + mn;
        g=mx;
        b=mn;
    }else if(h>=120&&h<180){
        r=mn;
        g=mx;
        b = ((h-120) * (mx-mn)) / 60 + mn;
    }else if(h>=180&&h<240){
        r=mn;
        g = ((240 - h) * (mx - mn)) / 60 + mn;
        b=mx;
    }else if(h>=240&&h<300){
        r = ((h-240) * (mx-mn)) / 60 + mn;
        g=mn;
        b=mx;
    }else if(h>=300&&h<360){
        r=mx;
        g=mn;
        b = ((360 - h) * (mx - mn)) / 60 + mn;
    }
    return (alpha|(r<<16)|(g<<8)|b);
}

int changeBrightness(int color, float ratio){
    int hsv = rgbTohsv(color);
    int v = (hsv & 0x000000FF);
    v *= ratio;
    if(v>255)v=255;
    return hsvTorgb((hsv & 0xFFFFFF00) | v);
}

void drawNumberBuff(int num, int subposY, int subposX){
    for(int y=0;y<5;y++){
        for(int x=0;x<4;x++){
            if(!(number[num][y][x] & 0x01000000)){
                screenBuff[subposY+y][subposX+x]=number[num][y][x];
            }
        }
    }
}

void drawTileBuff(int texture[BLOCK_Y][BLOCK_X], int posY, int posX){
    for(int y=0;y<BLOCK_Y;y++){
        for(int x=0;x<BLOCK_X;x++){
            screenBuff[posY * BLOCK_Y + y + FRAMEWIDTH][posX * BLOCK_X + x + FRAMEWIDTH] = texture[y][x];
        }
    }
}

void drawFrameBuff(Scene *scene){
    for(int i=0;i<FIELD_X+2;i++){
        drawTileBuff(frameTexture,-1,-1+i);
        drawTileBuff(frameTexture,FIELD_Y,-1+i);
    }
    for(int i=1;i<FIELD_Y+1;i++){
        drawTileBuff(frameTexture,-1+i,-1);
        drawTileBuff(frameTexture,-1+i,FIELD_X);
    }
}

void printScreen(){
    static int firstCall = 1;//初回呼び出しフラグ
    char screenTextBuff[BLOCK_X * FIELD_X * BLOCK_Y * FIELD_Y * 40];
    int numText = 0;
    numText += sprintf(screenTextBuff + numText, "\033[H");//カーソルを左上に移動
    for(int i = 0; i < (BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2) / 2; i ++){
        for(int j = 0; j < BLOCK_X * FIELD_X + FRAMEWIDTH * 2; j ++){
            if(firstCall || screenBuff[i * 2][j] != screenBuffPrev[i * 2][j] || screenBuff[i * 2 + 1][j] != screenBuffPrev[i * 2 + 1][j]){//初回または前回と異なるピクセルを検出
                if(screenBuff[i * 2][j] & 0x80000000){//文字フラグが立っていたら文字として扱う
                    if(screenBuff[i * 2][j] & 0x40000000){//その一個下のビットがあれば文字は黒色にする（X_X顔用）
                        numText += sprintf(screenTextBuff + numText, "\033[%d;%dH\033[38;2;0;0;0m\033[48;2;%d;%d;%dm%c",
                            i + 1,
                            j + 1,
                            ((screenBuff[i*2+1][j] & 0xFF0000) >> 16),
                            ((screenBuff[i*2+1][j] & 0x00FF00) >> 8),
                            (screenBuff[i*2+1][j] & 0x0000FF),
                            (char)(screenBuff[i * 2][j] & 0x000000FF));
                    }else{
                        numText += sprintf(screenTextBuff + numText, "\033[%d;%dH\033[38;2;255;255;255m\033[48;2;0;0;0m%c", i + 1, j + 1, (char)(screenBuff[i * 2][j] & 0x000000FF));
                    }
                }else{//そうでなければ色塗りする
                    numText += sprintf(screenTextBuff + numText,
                    "\033[%d;%dH\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀",
                    i + 1,
                    j + 1,
                    ((screenBuff[i*2][j] & 0xFF0000) >> 16),
                    ((screenBuff[i*2][j] & 0x00FF00) >> 8),
                    (screenBuff[i*2][j] & 0x0000FF),
                    ((screenBuff[i*2+1][j] & 0xFF0000) >> 16),
                    ((screenBuff[i*2+1][j] & 0x00FF00) >> 8),
                    (screenBuff[i*2+1][j] & 0x0000FF));
                }
            }
        }
    }
    numText += sprintf(screenTextBuff + numText, "\033[%d;1H", BLOCK_Y * FIELD_Y / 2 + 2);
    write(STDOUT_FILENO, screenTextBuff, numText);

    firstCall = 0;//初回フラグをオフ
    int (*screenBuffForSwap)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = screenBuffPrev;//screenBuffのポインタを入れ替える
    screenBuffPrev = screenBuff;
    screenBuff = screenBuffForSwap;
}
/*
エスケープシーケンスによる色付けの参考　：　https://www.mm2d.net/main/prog/c/console-03.html
エスケープシーケンスによるカーソル移動の参考　：　https://qiita.com/sudo00/items/2b2eec07d3099b5ad664
write関数について　：　https://cgengo.sakura.ne.jp/write.html
*/
