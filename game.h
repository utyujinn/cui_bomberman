#ifndef GAME_H
#define GAME_H
#include "types.h"
char get_abs_sub_x(Point *pos);
char get_abs_sub_y(Point *pos);
void init_world(World *world);
void process_frame(Scene *scene, char input_player1, char input_player2);
#endif