#include "types.h"
#include "graphics.h"
#include "texture.h"
#include <stdio.h>

#define TATIE_WIDTH 32
#define TATIE_HEIGHT 48


const char *descriptions[] = {
    "IRIE\n--from Univ.Tokyo EEIC--\nSkill : Can place quick-exploding bomb.\n Yourself don't die because of the bomb.",
    "KORI\n--from Nyakori's rabbit doll--\nSkill : Transform into a cat.\nWhile in cat form, you can move over the walls.",
    "MISAKI\n--the developer's original--\nSkill : Summon shooting stars of bomb.\n The bombs explode faster and smaller.",
    "TORU\n--from Nyakori's rabbit doll--\nSkill : Hold a fish.\nWhile holding a fish, you can \nattack directly by shaking the \nfish."
};


void drawMenuBuff(Scene* scene){
    for(int i = 0; i < TATIE_HEIGHT; i ++){
        for(int j = 0; j < TATIE_WIDTH; j ++){
            screenBuff[i+FRAMEWIDTH][j + 3 + FRAMEWIDTH] = tatie[scene->world.chosenName1][i][j];
            screenBuff[i+FRAMEWIDTH][j + TATIE_WIDTH + 5 + FRAMEWIDTH] = tatie[scene->world.chosenName2][i][j];
        }
    }
    for(int i = 0; i < BLOCK_Y; i ++){
        for(int j = 0; j < BLOCK_X; j ++){
            screenBuff[i + FRAMEWIDTH + TATIE_HEIGHT + 16][j + 10 + FRAMEWIDTH] = thumbnail[scene->world.chosenStage1][i][j];
            screenBuff[i + FRAMEWIDTH + TATIE_HEIGHT + 16][j + 29 + FRAMEWIDTH] = thumbnail[scene->world.chosenStage2][i][j];
        }
    }
    printText2Buff(scene, descriptions[scene->world.chosenName1], 3, TATIE_HEIGHT + 2, TATIE_WIDTH);
    printText2Buff(scene, descriptions[scene->world.chosenName2], TATIE_WIDTH + 5, TATIE_HEIGHT + 2, TATIE_WIDTH);
    printText2Buff(scene, "-----------------------------------------------------------------------", 0, FRAMEWIDTH + TATIE_HEIGHT + 6, 100);
    printText2Buff(scene, "P1's\nStage\nChoice:", 1, FRAMEWIDTH + TATIE_HEIGHT + 10, 100);
    printText2Buff(scene, "P2's\nStage\nChoice:", 20, FRAMEWIDTH + TATIE_HEIGHT + 10, 100);
    printText2Buff(scene, "Player select : A/D\nStage select : J/L\nConfiem : Enter", 40, FRAMEWIDTH + TATIE_HEIGHT + 10, 100);
    drawFrameBuff(scene);
    if(scene->flag){
        for(int i=0;i<BLOCK_Y*FIELD_Y+FRAMEWIDTH*2;i++){
            for(int j=0;j<BLOCK_X*FIELD_X+FRAMEWIDTH*2;j++){
                float ratio = 1.0-scene->timer2/30.0;
                screenBuff[i][j] = changeBrightness(screenBuff[i][j],ratio);
            }
        }
    }
}
