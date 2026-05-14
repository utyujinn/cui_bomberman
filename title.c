#include "types.h"
#include "game.h"
#include "client.h"

void process_frame_title(Scene *scene,char input,char player_id){
    scene->timer++;

    // 選択
    if(input=='d'&&scene->gamemode==0)scene->gamemode++;
    else if(input=='a'&&scene->gamemode==1)scene->gamemode--;
    else if(input=='s'&&scene->gamemode==0)scene->gamemode=2;
    else if(input=='w'&&scene->gamemode==2)scene->gamemode=0;
    else if(input=='s'&&scene->gamemode==1)scene->gamemode=2;
    else if(input=='d'&&scene->gamemode==2)scene->gamemode=1;
    else if(input=='a'&&scene->gamemode==2)scene->gamemode=0;

    // qを押すとゲーム終了フラグを立てる
    if(input=='q'){
        scene->flag=2;  // flag=2をゲーム終了の合図にする
        return;
    }

    if(input==' ' || input=='\n'){
        if(!(scene->gamemode&2)){scene->flag=1; scene->timer2=0;}
        else{
            scene->flag=0;
            scene->timer2=0;
            scene->state=STATE_HELP;
        }
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