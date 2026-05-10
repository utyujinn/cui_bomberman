#ifndef GAMEDATA_H
#define GAMEDATA_H

// gamedata.txtからユーザーのwinCountを読み込む
int load_win_count(const char* username);

// gamedata.txtのユーザーのwinCountを更新
int save_win_count(const char* username, int new_win_count);

#endif
