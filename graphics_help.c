#include "types.h"
#include "graphics.h"
#include "texture.h"
#include <stdio.h>

#define TATIE_WIDTH 32
#define TATIE_HEIGHT 48

void drawHelpBuff(Scene* scene){
    printText2Buff(scene, "--DESCRIPTION--\nMove : W/A/S/D\nBomb Place : Space/Shift\n*note* You can place up to 2 bombs by default.\nSkill Activate/Action : Q/Enter", 2, 2, 100);
    printText2Buff(scene, "This item makes your speed faster.\n1 item = x2 speed, 2 item = x4 speed.\nEven if you get more, you can't become faster.", 12, 14, 100);
    printText2Buff(scene, "This item increases your bomb placement limit.", 12, 24, 100);
    printText2Buff(scene, "This item extends the length your bonb's fire can reach.\nThis doesn't affect the bombs placed by your skill.", 12, 34, 100);

    printText2Buff(scene, "Created by :\n- Utyujinn\n- Amatsu Itadaki", 2, 56, 100);


    for(int i = 0; i < BLOCK_Y; i ++){
        for(int j = 0; j < BLOCK_X; j ++){
            screenBuff[i + 22][j + 2 + FRAMEWIDTH] = itemA[i][j];
            screenBuff[i + 32][j + 2 + FRAMEWIDTH] = itemB[i][j];
            screenBuff[i + 42][j + 2 + FRAMEWIDTH] = itemC[i][j];
        }
    }



    drawFrameBuff(scene);
}
