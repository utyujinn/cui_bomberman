#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gamedata.h"

// gamedata.txtからユーザーのwinCountを読み込む
int load_win_count(const char* username) {
    FILE *fp = fopen("gamedata.txt", "r");
    if (fp == NULL) {
        // ファイルが存在しない場合は0を返す
        return 0;
    }

    char line[256];
    char file_username[128];
    int win_count;

    while (fgets(line, sizeof(line), fp) != NULL) {
        // "User=100" の形式をパース
        if (sscanf(line, "%127[^=]=%d", file_username, &win_count) == 2) {
            if (strcmp(file_username, username) == 0) {
                fclose(fp);
                if(win_count<0)win_count=0;
                if(win_count>999)win_count=999;
                return win_count;
            }
        }
    }

    fclose(fp);
    return 0; // ユーザーが見つからない場合は0を返す
}

// gamedata.txtのユーザーのwinCountを更新
int save_win_count(const char* username, int new_win_count) {
    FILE *fp = fopen("gamedata.txt", "r");
    char lines[100][256];
    int line_count = 0;
    int user_found = 0;
    if(new_win_count<0)new_win_count=0;
    if(new_win_count>999)new_win_count=999;

    // 既存の内容を読み込む
    if (fp != NULL) {
        while (fgets(lines[line_count], sizeof(lines[line_count]), fp) != NULL && line_count < 100) {
            char file_username[128];
            int win_count;

            // この行が更新対象のユーザーかチェック
            if (sscanf(lines[line_count], "%127[^=]=%d", file_username, &win_count) == 2) {
                if (strcmp(file_username, username) == 0) {
                    // ユーザーが見つかったので更新
                    snprintf(lines[line_count], sizeof(lines[line_count]), "%s=%d\n", username, new_win_count);
                    user_found = 1;
                }
            }
            line_count++;
        }
        fclose(fp);
    }

    // ユーザーが見つからなかった場合は追加
    if (!user_found) {
        snprintf(lines[line_count], sizeof(lines[line_count]), "%s=%d\n", username, new_win_count);
        line_count++;
    }

    // ファイルに書き戻す
    fp = fopen("gamedata.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open gamedata.txt for writing\n");
        return -1;
    }

    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], fp);
    }

    fclose(fp);
    return 0;
}
