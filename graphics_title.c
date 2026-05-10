#include "types.h"
#include "graphics.h"
#include "texture.h"

void drawTitleBGBuff(Scene* scene){
    for(int i=0;i<TITLE_HEIGHT;i++){
        for(int j=0;j<TITLE_WIDTH;j++){
            screenBuff[i+TITLE_OFFSET_Y][j+TITLE_OFFSET_X] = titleTexture[i][j];
        }
    }
}

void drawSelectionBGBuff(char selected, char subposy, char subposx){
    for(int i=0;i<13;i++){
        for(int j=0;j<27;j++){
            if(!(modeBGTexture[selected][i][j]>>24 & 0x01)){
                screenBuff[subposy+i][subposx+j] = modeBGTexture[selected][i][j];
            }
        }
    }
}

void drawSelectionFGBuff(char num, char subposy, char subposx){
    for(int i=0;i<11;i++){
        for(int j=0;j<25;j++){
            if(!(modeTexture[num][i][j]>>24 & 0x01)){
                screenBuff[subposy+i+1][subposx+j+1] = modeTexture[num][i][j];
            }
        }
    }
}

void drawSelectionBuff(Scene* scene){
    if(scene->gamemode==0)drawSelectionBGBuff(1,50,17);
    else drawSelectionBGBuff(0,50,17);
    if(scene->gamemode==1)drawSelectionBGBuff(1,50,46);
    else drawSelectionBGBuff(0,50,46);
    if(scene->gamemode==2)drawSelectionBGBuff(1,66,30);
    else drawSelectionBGBuff(0,66,30);
    drawSelectionFGBuff(0,50,17);
    drawSelectionFGBuff(1,50,46);
    drawSelectionFGBuff(2,66,30);
}

void drawTitleBuff(Scene* scene){
    drawFrameBuff(scene);
    drawTitleBGBuff(scene);
    drawSelectionBuff(scene);
    if(scene->flag){
        for(int i=0;i<BLOCK_Y*FIELD_Y+FRAMEWIDTH*2;i++){
            for(int j=0;j<BLOCK_X*FIELD_X+FRAMEWIDTH*2;j++){
                float ratio = 1.0-scene->timer2/30.0;
                screenBuff[i][j] = changeBrightness(screenBuff[i][j],ratio);
            }
        }
    }
    /*
    if(scene->gamemode==1){
        for(int i=0;i<TITLE_HEIGHT/2;i++){
                for(int j=0;j<TITLE_WIDTH/2;j++){
                    float ratio = 1.0-scene->timer2/30.0;
                    screenBuff[i+TITLE_OFFSET_Y+TITLE_HEIGHT/4][j+TITLE_OFFSET_X+TITLE_WIDTH/4] = changeBrightness(titleTexture[0][i][j],ratio);
                }
            }
    }*/
}
