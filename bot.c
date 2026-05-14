#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "game.h"

// BFS用のキュー構造
typedef struct {
    int x, y;
    int dist;
    char first_move;  // 最初の移動方向
} QueueNode;

static int repeatkey=0;
static char lastkey=' ';

// 危険度マップの構築
// 安全0 アイテム1 爆弾範囲(余裕)2 爆弾本体(通行不可)3 石中心4
// 直前爆弾範囲(物理通行可・危険)5  火/壁/石隅(物理通行不可)6
void build_danger_map(World *world, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    // 初期化
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)danger[i][j]=0;

    // 壁と石ブロック
    for(int y = 0; y < FIELD_Y; y++) {
        for(int x = 0; x < FIELD_X; x++) {
            if(world->field[y][x].kind == 1){//壁ブロック
                for(int i=y*4-3;i<=y*4+3;i++) {
                    for(int j=x*4-3;j<=x*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3) {
                            danger[i][j] = 6;  // 物理通行不可（壁）
                        }
                    }
                }
            }
        }
    }
    for(int y=0; y< FIELD_Y;y++){
        for(int x=0; x< FIELD_X;x++){
            if(world->field[y][x].kind == 2) {//石
                for(int i=y*4-3;i<=y*4+3;i++) {
                    for(int j=x*4-3;j<=x*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3) {
                            if(i>y*4-1&&i<y*4+1)danger[i][j] = 4;  // 石中心（通行不可）
                            else if(j>x*4-1&&j<x*4+1)danger[i][j] = 4;
                            else danger[i][j]=6;  // 石隅（物理通行不可）
                        }
                    }
                }
            }
        }
    }

    // スプライト（アイテム、爆弾と火）をチェック
    for(int i = 0; i < MAX_SPRITES_NUM; i++) {
        Sprite *s = &world->sprites[i];

        // アイテム（A, B, C）をdanger=1に設定
        if(s->kind == ITEM_A || s->kind == ITEM_B || s->kind == ITEM_C) {
            int item_x = s->pos.x;
            int item_y = s->pos.y;
            for(int i=item_y*4-3;i<=item_y*4+3;i++) {
                for(int j=item_x*4-3;j<=item_x*4+3;j++) {
                    if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3) {
                        if(i>item_y*4-1&&i<item_y*4+1 && j>item_x*4-1&&j<item_x*4+1) {
                            danger[i][j] = 1;  // アイテム
                        }
                    }
                }
            }
        }

        // 火（現在発生中：物理通行不可として扱う）
        if(s->kind == FIRE) {
            int fx = s->pos.x;
            int fy = s->pos.y;
            for(int i=fy*4-3;i<=fy*4+3;i++) {
                for(int j=fx*4-3;j<=fx*4+3;j++) {
                    if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3) {
                        danger[i][j] = 6;  // 物理通行不可（火）
                    }
                }
            }
        }

        // 爆弾の爆発予測範囲
        if(s->kind == BOMB) {
            int bx = s->pos.x;
            int by = s->pos.y;

            // ボット(player2)が通り抜けられるかチェック
            // statusの第1ビットがplayer2の通り抜けフラグ
            int can_pass_through = (s->status >> 1) & 0x01;
            int bomb_danger = can_pass_through ? 2 : 3;  // 通り抜け可能:2, 不可:3

            // プレイヤーのstatusから火の長さを取得（デフォルト2）
            int range = 2;
            if(s->owner == 1) {
                range = (world->player1.status >> 2) & 0x07;
            } else if(s->owner == 2) {
                range = (world->player2.status >> 2) & 0x07;
            }

            // 直前爆弾(5)は物理的には通れるが危険、余裕爆弾(2)は低危険
            int fire_danger = (s->timer <= 20) ? 5 : 2;
            // 上方向
            for(int r = 1; r <= range; r++) {
                int cell_y = by - r;
                if(cell_y < 0 || world->field[cell_y/4][bx].kind == 1) break;

                for(int i=cell_y*4-3;i<=cell_y*4+3;i++) {
                    for(int j=bx*4-3;j<=bx*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3 && danger[i][j] < 2) {
                            danger[i][j] = fire_danger;
                        }
                    }
                }

                if(world->field[cell_y/4][bx].kind == 2) break;
            }

            // 下方向
            for(int r = 1; r <= range; r++) {
                int cell_y = by + r;
                if(cell_y >= FIELD_Y || world->field[cell_y][bx].kind == 1) break;

                for(int i=cell_y*4-3;i<=cell_y*4+3;i++) {
                    for(int j=bx*4-3;j<=bx*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3 && danger[i][j] < 2) {
                            danger[i][j] = fire_danger;
                        }
                    }
                }

                if(world->field[cell_y][bx].kind == 2) break;
            }

            // 左方向
            for(int r = 1; r <= range; r++) {
                int cell_x = bx - r;
                if(cell_x < 0 || world->field[by][cell_x].kind == 1) break;

                for(int i=by*4-3;i<=by*4+3;i++) {
                    for(int j=cell_x*4-3;j<=cell_x*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3 && danger[i][j] < 2) {
                            danger[i][j] = fire_danger;
                        }
                    }
                }

                if(world->field[by][cell_x].kind == 2) break;
            }

            // 右方向
            for(int r = 1; r <= range; r++) {
                int cell_x = bx + r;
                if(cell_x >= FIELD_X || world->field[by][cell_x].kind == 1) break;

                for(int i=by*4-3;i<=by*4+3;i++) {
                    for(int j=cell_x*4-3;j<=cell_x*4+3;j++) {
                        if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3 && danger[i][j] < 2) {
                            danger[i][j] = fire_danger;
                        }
                    }
                }
                if(world->field[by][cell_x].kind == 2) break;
            }

            // 爆弾の位置（通り抜けフラグに応じて危険度を設定）
            for(int i=by*4-3;i<=by*4+3;i++) {
                for(int j=bx*4-3;j<=bx*4+3;j++) {
                    if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3 && danger[i][j] < bomb_danger) {
                        danger[i][j] = bomb_danger;
                    }
                }
            }
        }
    }
}

// BFSで最も近い安全なアイテムを探す（danger=1の場所を探す）
char find_nearest_item(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    QueueNode queue[FIELD_X * FIELD_Y*16];
    int front = 0, rear = 0;
    int visited[FIELD_Y*4-3][FIELD_X*4-3];
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)visited[i][j]=0;

    queue[rear++] = (QueueNode){bot_x, bot_y, 0, 0};
    visited[bot_y][bot_x] = 1;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    while(front < rear) {
        QueueNode curr = queue[front++];

        // danger=1の場所を発見（アイテムがある場所）
        if(danger[curr.y][curr.x] == 1) {
            return curr.first_move;
        }

        // 4方向に探索
        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];

            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3 && !visited[ny][nx] && danger[ny][nx] < 3) {
                visited[ny][nx] = 1;
                char first = curr.first_move ? curr.first_move : moves[i];
                queue[rear++] = (QueueNode){nx, ny, curr.dist + 1, first};
            }
        }
    }

    return 0;  // アイテムが見つからない
}

// BFSで最も近い石ブロックを探す
char find_nearest_stone(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    QueueNode queue[FIELD_X * FIELD_Y*16];
    int front = 0, rear = 0;
    int visited[FIELD_Y*4-3][FIELD_X*4-3];
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)visited[i][j]=0;

    queue[rear++] = (QueueNode){bot_x, bot_y, 0, 0};
    visited[bot_y][bot_x] = 1;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    while(front < rear) {
        QueueNode curr = queue[front++];

        // 隣接する石を発見
        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];
            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3) {
                if(danger[ny][nx] == 4) {  // 石ブロック
                    return curr.first_move ? curr.first_move : moves[i];
                }
            }
        }

        // 4方向に探索
        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];

            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3 && !visited[ny][nx] && danger[ny][nx] < 3) {
                visited[ny][nx] = 1;
                char first = curr.first_move ? curr.first_move : moves[i];
                queue[rear++] = (QueueNode){nx, ny, curr.dist + 1, first};
            }
        }
    }

    return 0;  // 石が見つからない
}

// BFSで相手プレイヤーに向かう
char move_toward_player(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    Player *enemy = &world->player1;
    char enemy_x = get_abs_sub_x(&enemy->pos);
    char enemy_y = get_abs_sub_y(&enemy->pos);
    enemy_x >>= 1;
    enemy_y >>= 1;

    QueueNode queue[FIELD_X * FIELD_Y*16];
    int front = 0, rear = 0;
    int visited[FIELD_Y*4-3][FIELD_X*4-3];
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)visited[i][j]=0;

    queue[rear++] = (QueueNode){bot_x, bot_y, 0, 0};
    visited[bot_y][bot_x] = 1;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    while(front < rear) {
        QueueNode curr = queue[front++];

        // 相手プレイヤーの近くに到達
        int dist_to_enemy = abs(curr.x - enemy_x) + abs(curr.y - enemy_y);
        if(dist_to_enemy <= 4) {  // 相手に近づいた
            return curr.first_move;
        }

        // 4方向に探索
        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];

            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3 && !visited[ny][nx] && danger[ny][nx] < 3) {
                visited[ny][nx] = 1;
                char first = curr.first_move ? curr.first_move : moves[i];
                queue[rear++] = (QueueNode){nx, ny, curr.dist + 1, first};
            }
        }
    }

    return 0;  // 相手に向かえない
}

// BFSで最も近い安全地帯を探す
char find_safe_zone(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    QueueNode queue[FIELD_X * FIELD_Y*16];
    int front = 0, rear = 0;
    int visited[FIELD_Y*4-3][FIELD_X*4-3];
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)visited[i][j]=0;

    queue[rear++] = (QueueNode){bot_x, bot_y, 0, 0};
    visited[bot_y][bot_x] = 1;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    while(front < rear) {
        QueueNode curr = queue[front++];

        // 安全地帯を発見 (danger == 0)
        if(danger[curr.y][curr.x] <= 1) {
            return curr.first_move;
        }

        // 4方向に探索
        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];
            //石では無い時動ける、最初の移動の時、移動方向を保存
            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3 && !visited[ny][nx] && (danger[ny][nx]<=2)) {
                visited[ny][nx] = 1;
                char first = curr.first_move ? curr.first_move : moves[i];
                queue[rear++] = (QueueNode){nx, ny, curr.dist + 1, first};
            }
        }
    }

    return 0;  // 安全地帯が見つからない
}

// 直前爆弾マス(danger=5)も通り抜けて最寄りの安全地帯を探す
// 通常BFSが失敗（danger=5に囲まれた）場合の最終手段
// 火(6)・壁(6)・石(4)・爆弾本体(3)は通らない
char find_safe_zone_desperate(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    QueueNode queue[FIELD_X * FIELD_Y * 16];
    int front = 0, rear = 0;
    int visited[FIELD_Y*4-3][FIELD_X*4-3];
    for(int i=0;i<FIELD_Y*4-3;i++)for(int j=0;j<FIELD_X*4-3;j++)visited[i][j]=0;

    queue[rear++] = (QueueNode){bot_x, bot_y, 0, 0};
    visited[bot_y][bot_x] = 1;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    while(front < rear) {
        QueueNode curr = queue[front++];

        if(danger[curr.y][curr.x] <= 1) {
            return curr.first_move ? curr.first_move : 0;
        }

        for(int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];
            // bounds → visited → 通行可否 の順でチェック（短絡評価を利用）
            // danger<=2(通常通行可) または danger==5(直前爆弾、物理的には通れる) を許可
            // danger==3(爆弾本体), 4(石中心), 6(火/壁) は通らない
            if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3 &&
               !visited[ny][nx] &&
               (danger[ny][nx] <= 2 || danger[ny][nx] == 5)) {
                visited[ny][nx] = 1;
                char first = curr.first_move ? curr.first_move : moves[i];
                queue[rear++] = (QueueNode){nx, ny, curr.dist + 1, first};
            }
        }
    }

    return 0;
}

// 隣接する石ブロックをチェック
int has_adjacent_stone(World *world, int x, int y,int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for(int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if(nx >= 0 && nx < FIELD_X*4-3 && ny >= 0 && ny < FIELD_Y*4-3) {
            if(danger[ny][nx] == 4) {
                return 1;
            }
        }
    }
    return 0;
}

// 爆弾を置いた後に逃げられるかチェック
int can_escape_after_bomb(World *world, int bot_x, int bot_y, int danger[FIELD_Y*4-3][FIELD_X*4-3]) {
    // 現在位置に爆弾を置いたと仮定して、逃げ道があるかチェック
    int temp_danger[FIELD_Y*4-3][FIELD_X*4-3];
    memcpy(temp_danger, danger, sizeof(temp_danger));

    // 仮想的に爆弾の爆発範囲を追加
    for(int i=bot_y-3;i<=bot_y+3;i++) {
        for(int j=bot_x-3;j<=bot_x+3;j++) {
            if(i >= 0 && i < FIELD_Y*4-3 && j >= 0 && j < FIELD_X*4-3) {
                temp_danger[i][j] = 2;
            }
        }
    }

    int range = (world->player2.status >> 2) & 0x07;  // ボットの火の長さ
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for(int i = 0; i < 4; i++) {
        for(int r = 1; r <= range; r++) {
            int nx = bot_x + dx[i] * r * 4;
            int ny = bot_y + dy[i] * r * 4;
            if(nx < 0 || nx >= FIELD_X*4-3 || ny < 0 || ny >= FIELD_Y*4-3) break;
            if(world->field[ny][nx].kind == 1) break;
            for(int i=ny-3;i<=ny+3;i++) for(int j=nx-3;j<=nx+3;j++)if(i>=0&&i<FIELD_Y*4-3&&j>=0&&j<FIELD_X*4-3)if(temp_danger[ny][nx] < 2)temp_danger[i][j] = 2;
            if(world->field[ny][nx].kind == 2) break;
        }
    }

    // 安全地帯が見つかるかチェック
    char escape = find_safe_zone(world, bot_x, bot_y, temp_danger);
    return (escape != 0);
}

char generate_menu_input(Scene *scene){
    if(scene->timer % 60 == 1){
        int action = rand() % 2;
        if(action == 0) {
            return 'd';
        } else {
            return 'a';
        }
    }else if(scene->timer % 60 == 31){
        int action = rand() % 2;
        if(action == 0) {
            return 'l';
        } else {
            return 'j';
        }
    }
    return 0;
}


char generate_game_input(Scene *scene){
    static int skill_used=0;
    if(scene->timer3 > 900 && skill_used==0){
        skill_used=1;
        return '\n';
    }
    World *world = &scene->world;
    Player *bot = &world->player2;

    char bot_x = get_abs_sub_x(&bot->pos);
    char bot_y = get_abs_sub_y(&bot->pos);
    bot_x>>=1;
    bot_y>>=1;

    // speed=4のとき subx=4 → BFS x%4==2 になると ±4移動で永遠に脱出できない
    // BFS起点を最寄りの4の倍数にスナップしてBFSに上下方向を返させる
    // game.cのsubx==4スナップが実際の物理整合を担当する
    if((bot->status & 0x03) == 2) {
        if(bot_x % 4 == 2) bot_x -= 2;
        if(bot_y % 4 == 2) bot_y -= 2;
    }

    // 危険度マップを構築
    int danger[FIELD_Y*4-3][FIELD_X*4-3];
    build_danger_map(world, danger);

    // 優先度1: 危険回避 - 現在地が危険なら逃げる
    if(danger[bot_y][bot_x] >= 2) {
        char escape = find_safe_zone(world, bot_x, bot_y, danger);
        if(escape) return escape;
        // 通常BFS失敗（直前爆弾に囲まれた）→ 直前爆弾マスを通り抜けてでも逃げる
        char desperate = find_safe_zone_desperate(world, bot_x, bot_y, danger);
        if(desperate) return desperate;
    }

    // 優先度2: アイテム収集 - 安全なアイテムを取りに行く
    char item_move = find_nearest_item(world, bot_x, bot_y, danger);
    if(item_move) return item_move;

    // 優先度3: 敵を狙った爆弾設置 - 距離8以内に敵がいて逃げられるなら爆弾を置く
    if(bot->bombStock > 0) {
        Player *enemy = &world->player1;
        char enemy_x = get_abs_sub_x(&enemy->pos);
        char enemy_y = get_abs_sub_y(&enemy->pos);
        enemy_x >>= 1;
        enemy_y >>= 1;

        int dist_to_enemy = abs(bot_x - enemy_x) + abs(bot_y - enemy_y);
        if(dist_to_enemy <= 8 && can_escape_after_bomb(world, bot_x, bot_y, danger)) {
            // 爆弾の爆発範囲に敵が入るかチェック
            int range = (bot->status >> 2) & 0x07;
            int enemy_in_range = 0;

            // 上下左右の爆発範囲をチェック
            if(bot_x == enemy_x && abs(bot_y - enemy_y) <= range) enemy_in_range = 1;
            if(bot_y == enemy_y && abs(bot_x - enemy_x) <= range) enemy_in_range = 1;

            if(enemy_in_range) {
                return ' ';  // 爆弾を置く
            }
        }
    }

    // 優先度4: 壁破壊 - 隣に石ブロックがあり、逃げられるなら爆弾を置く
    if(bot->bombStock > 0 && has_adjacent_stone(world, bot_x, bot_y, danger)) {
        if(can_escape_after_bomb(world, bot_x, bot_y, danger)) {
            return ' ';  // 爆弾を置く
        }
    }

    // 優先度5: 石に向かって移動 - 最も近い石ブロックに向かう
    char stone_move = find_nearest_stone(world, bot_x, bot_y, danger);
    if(stone_move) return stone_move;

    // 優先度6: 相手プレイヤーに向かう
    char player_move = move_toward_player(world, bot_x, bot_y, danger);
    if(player_move) return player_move;

    // 優先度7: ランダム移動（安全な方向）
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    char moves[] = {'w', 's', 'a', 'd'};

    for(int i = 0; i < 4; i++) {
        int a = rand()%4;
        int nx = bot_x + dx[(a+i)%4];
        int ny = bot_y + dy[(a+i)%4];
        if(nx >= 0 && nx < FIELD_X*4 && ny >= 0 && ny < FIELD_Y*4) {
            if(danger[ny][nx] <= 1) {
                return moves[(a+i)%4];
            }
        }
    }

    return 0;
}

char generate_bot_input(Scene *scene) {
    if(BOT_SPEED==1?1:scene->timer%BOT_SPEED==1){
        if(scene->state == STATE_MENU) {
            return generate_menu_input(scene);
        } else if(scene->state == STATE_PLAY) {
            char newkey = generate_game_input(scene);
            if(newkey==lastkey)repeatkey++;
            lastkey=newkey;
            return newkey;
        }
    } else {
        return 0;
    }
}