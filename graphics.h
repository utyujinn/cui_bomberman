#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "types.h"

// 共通の描画バッファ（extern宣言）
extern int screenBuff1[BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2][BLOCK_X * FIELD_X + FRAMEWIDTH * 2];
extern int screenBuff2[BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2][BLOCK_X * FIELD_X + FRAMEWIDTH * 2];
extern int (*screenBuff)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2];
extern int (*screenBuffPrev)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2];

// 共通関数
void drawBlack();
void printText2Buff(Scene* scene, const char* text, char x, char y, char width);
int rgbTohsv(int color);      // RGBからHSVへ変換
int hsvTorgb(int color);      // HSVからRGBへ変換
int changeBrightness(int color, float ratio); // 明るさを変更
void drawNumberBuff(int num, int subposY, int subposX);
void drawFrameBuff(Scene *scene); // フレームを描画
void printScreen();                // バッファを画面に出力

#endif
