#ifndef TEXTURE_H
#define TEXTURE_H
#include "types.h"

//externについてhttps://teratail.com/questions/353218

extern int titleTexture[TITLE_HEIGHT][TITLE_WIDTH];

extern int tatie[4][48][32];

extern int itemA[BLOCK_Y][BLOCK_X];

extern int itemB[BLOCK_Y][BLOCK_X];

extern int itemC[BLOCK_Y][BLOCK_X];

extern int floor_texture[2][5][BLOCK_Y][BLOCK_X];

extern int wall[2][8][8];

extern int stone[2][8][8];

extern int thumbnail[4][8][8];

extern int bomb[10][8][8];

extern int fire[6][8][8];

extern int player[4][14][8][8];

extern int number[11][5][4];

extern int resultTexture[RESULT_HEIGHT][RESULT_WIDTH];

extern int modeTexture[3][11][25];

extern int modeBGTexture[2][13][27];

extern int frameTexture[8][8];

#endif 