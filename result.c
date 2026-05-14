#include "types.h"
#include "game.h"

void process_frame_result(Scene *scene,char input_player1,char input_player2){
    scene->timer++;

    // qを押すと即座にタイトルに戻る
    if(input_player1=='q' || input_player2=='q'){
        scene->flag=3;  // flag=3をサーバー離脱の合図にする
        scene->state=STATE_TITLE;
        return;
    }

    if(input_player1=='\n' || input_player2=='\n' || input_player1==' ' || input_player2==' '){
        scene->flag=1;
        scene->timer2=0;
    }
    if(scene->flag==1){
        scene->timer2++;
        if(scene->timer2>30){
            scene->flag=0;
            scene->timer2=0;
            scene->state=STATE_MENU;
        }
    }
}