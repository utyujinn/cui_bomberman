#include "types.h"
#include "graphics.h"
#include "texture.h"

void drawBGBuff(Scene* scene){
    if(scene->flag==1){
        for(int i=0;i<RESULT_HEIGHT;i++){
            for(int j=0;j<RESULT_WIDTH;j++){
                float ratio = 1.0-scene->timer2/30.0;
                screenBuff[i+RESULT_OFFSET_Y][j+RESULT_OFFSET_X] = changeBrightness(resultTexture[i][j],ratio);
            }
        }
    }else{
        for(int i=0;i<RESULT_HEIGHT;i++){
            for(int j=0;j<RESULT_WIDTH;j++){
                screenBuff[i+RESULT_OFFSET_Y][j+RESULT_OFFSET_X] = resultTexture[i][j];
            }
        }
    }
}

void drawPlayerBuffxratio(Scene *scene,char player_id,char ratio,char subposY, char subposX){
    char playername;
    char isAlive;
    if(player_id==1){
        playername = scene->world.player1.name;
        isAlive = scene->world.player1.isAlive;
    }else if(player_id==2){
        playername = scene->world.player2.name;
        isAlive = scene->world.player2.isAlive;
    }
    if(isAlive){
        for(int y=0;y<BLOCK_Y;y++){
            for(int x=0;x<BLOCK_X;x++){
                if(!(player[playername][(scene->timer>>4)%5][y][x] & 0x01000000)){
                    for(int i=0;i<ratio;i++){
                        for(int j=0;j<ratio;j++){
                            screenBuff[subposY+y*ratio+i][subposX+x*ratio+j]= player[playername][(scene->timer>>4)%5][y][x];
                        }
                    }
                }
            }
        }
    }else{
        for(int y=0;y<BLOCK_Y;y++){
            for(int x=0;x<BLOCK_X;x++){
                if(!(player[playername][9][y][x] & 0x01000000)){
                    for(int i=0;i<ratio;i++){
                        for(int j=0;j<ratio;j++){
                            screenBuff[subposY+y*ratio+i][subposX+x*ratio+j]= player[playername][9][y][x];
                        }
                    }
                }
            }
        }//(4,2)に(5,2)に色情報、()
        if(ratio>=2){
            int color=player[playername][9][5][2];
            for(int i=0;i<ratio*2;i++){
                for(int j=0;j<ratio;j++){
                    screenBuff[subposY+i+ratio*4][subposX+j+ratio*2]=color;
                }
            }
            for(int i=0;i<ratio*2;i++){
                for(int j=0;j<ratio;j++){
                    screenBuff[subposY+i+ratio*4][subposX+j+ratio*5]=color;
                }
            }
        }
        if(ratio==2){
            screenBuff[subposY+ratio*4][subposX+ratio*2]=0x00000000;
            screenBuff[subposY+ratio*4+1][subposX+ratio*2+1]=0x00000000;
            screenBuff[subposY+ratio*4+2][subposX+ratio*2+1]=0x00000000;
            screenBuff[subposY+ratio*4+3][subposX+ratio*2]=0x00000000;
            screenBuff[subposY+ratio*4][subposX+ratio*5+1]=0x00000000;
            screenBuff[subposY+ratio*4+1][subposX+ratio*5]=0x00000000;
            screenBuff[subposY+ratio*4+2][subposX+ratio*5]=0x00000000;
            screenBuff[subposY+ratio*4+3][subposX+ratio*5+1]=0x00000000;
        }else if(ratio==3){
            screenBuff[subposY+ratio*4][subposX+ratio*2]=0x00000000;
            screenBuff[subposY+ratio*4+1][subposX+ratio*2+1]=0x00000000;
            screenBuff[subposY+ratio*4+2][subposX+ratio*2+2]=0x00000000;
            screenBuff[subposY+ratio*4+3][subposX+ratio*2+2]=0x00000000;
            screenBuff[subposY+ratio*4+4][subposX+ratio*2+1]=0x00000000;
            screenBuff[subposY+ratio*4+5][subposX+ratio*2]=0x00000000;
            screenBuff[subposY+ratio*4][subposX+ratio*5+2]=0x00000000;
            screenBuff[subposY+ratio*4+1][subposX+ratio*5+1]=0x00000000;
            screenBuff[subposY+ratio*4+2][subposX+ratio*5]=0x00000000;
            screenBuff[subposY+ratio*4+3][subposX+ratio*5]=0x00000000;
            screenBuff[subposY+ratio*4+4][subposX+ratio*5+1]=0x00000000;
            screenBuff[subposY+ratio*4+5][subposX+ratio*5+2]=0x00000000;
        }
    }
}

void drawPlayerBuffForResult(Scene *scene){
    //player1
    char player1name = scene->world.player1.name;
    char player2name = scene->world.player2.name;
    int p1win = scene->world.player1.winCount;
    int p2win = scene->world.player2.winCount;
    char p13 = p1win/100;
    char p12 = (p1win/10)%10;
    char p11 = (p1win)%10;
    char p23 = p2win/100;
    char p22 = (p2win/10)%10;
    char p21 = (p2win)%10;

    if(p13){
        drawNumberBuff(p13,38,15);
        drawPlayerBuffxratio(scene,1,3,20,19);
        drawNumberBuff(p12,38,44);
        drawPlayerBuffxratio(scene,1,2,28,48);
        drawNumberBuff(p11,38,65);
        drawPlayerBuffxratio(scene,1,1,36,69);
    }
    else if(p12){
        drawNumberBuff(p12,38,44);
        drawPlayerBuffxratio(scene,1,2,28,48);
        drawNumberBuff(p11,38,65);
        drawPlayerBuffxratio(scene,1,1,36,69);
    }
    else{
        drawNumberBuff(p11,38,65);
        drawPlayerBuffxratio(scene,1,1,36,69);
    }
    if(p23){
        drawNumberBuff(p23,65,15);
        drawPlayerBuffxratio(scene,2,3,46,19);
        drawNumberBuff(p22,65,44);
        drawPlayerBuffxratio(scene,2,2,54,48);
        drawNumberBuff(p21,65,65);
        drawPlayerBuffxratio(scene,2,1,62,69);
    }
    else if(p22){
        drawNumberBuff(p22,65,44);
        drawPlayerBuffxratio(scene,2,2,54,48);
        drawNumberBuff(p21,65,65);
        drawPlayerBuffxratio(scene,2,1,62,69);
    }
    else{
        drawNumberBuff(p21,65,65);
        drawPlayerBuffxratio(scene,2,1,62,69);
    }
}

void drawResultBuff(Scene* scene){
    drawFrameBuff(scene);
    drawBGBuff(scene);
    drawPlayerBuffForResult(scene);
}
