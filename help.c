#include "types.h"
#include "game.h"

void process_frame_help(Scene *scene,char input_player1,char input_player2){
    scene->timer++;

    if(input_player1 == 'q'){
        scene->state = STATE_TITLE;
        return;
    }
}
