#include "types.h"
#include "game.h"

void process_frame_menu(Scene *scene,char input_player1,char input_player2){
    scene->timer++;

    if(input_player1 == 'd' && scene->world.chosenName1 < 3){
        scene->world.chosenName1 ++;
    }
    if(input_player1 == 'a' && scene->world.chosenName1 > 0){
        scene->world.chosenName1 --;
    }
    if(input_player2 == 'd' && scene->world.chosenName2 < 3){
        scene->world.chosenName2 ++;
    }
    if(input_player2 == 'a' && scene->world.chosenName2 > 0){
        scene->world.chosenName2 --;
    }

    if(input_player1 == 'l' && scene->world.chosenStage1 < 3){
        scene->world.chosenStage1 ++;
    }
    if(input_player1 == 'j' && scene->world.chosenStage1 > 0){
        scene->world.chosenStage1 --;
    }
    if(input_player2 == 'l' && scene->world.chosenStage2 < 3){
        scene->world.chosenStage2 ++;
    }
    if(input_player2 == 'j' && scene->world.chosenStage2 > 0){
        scene->world.chosenStage2 --;
    }

    if(input_player1=='\n' || input_player2=='\n' || input_player1==' ' || input_player2==' '){
        scene->flag=1;
        scene->timer2=0;
    }
    if(scene->flag){
        scene->timer2++;
        if(scene->timer2>30){
            scene->flag=0;
            scene->timer2=0;
            scene->state=STATE_PLAY;
            init_world(&scene->world);
        }
    }
}
