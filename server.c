#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "types.h"
#include "game.h"
#include "title.h"
#include "menu.h"
#include "result.h"
#include "bot.h"
#include "config.h"

#define PORT 8000
#define MAX_CLIENTS 2  // 最大2人まで接続可能

/*
参考文献
システムプログラミング入門 渡辺知恵美著 サイエンス社
pthread,mutexなど

libwebsockets公式ドキュメント
https://libwebsockets.org

simple-libwebsockets-example
https://github.com/iamscottmoyers/simple-libwebsockets-example/tree/master

claude code
*/

struct client_info {
    struct lws *wsi;
    char name[64];
    int player_id;  // 1 or 2
    int win_count;  // クライアントから受信した勝利数
    int initialized; // INITメッセージを受信済みかどうか
};

static struct client_info clients[MAX_CLIENTS];
static int client_count = 0;
static Scene server_scene;
static pthread_mutex_t scene_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
static int client1_connected_time = 0;
static char bot_active = 0;
static char notify_use=0;
static char log_use=0;

// クライアント管理
static void add_client(struct lws *wsi) {
    pthread_mutex_lock(&client_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count].wsi = wsi;
        clients[client_count].player_id = (client_count % 2) + 1;  // 1 or 2
        clients[client_count].win_count = 0;
        clients[client_count].initialized = 0;  // まだINITメッセージを受信していない
        snprintf(clients[client_count].name, sizeof(clients[client_count].name),
                "Player%d", clients[client_count].player_id);
        client_count++;
        if(client_count == 1 && notify_use){
            char access_token[256] = "";
            readconfig_server(".env", access_token);
            if(strlen(access_token) > 0){
                char command[2048];
                snprintf(command, sizeof(command),
                    "curl -X POST https://api.line.me/v2/bot/message/broadcast "
                    "-H 'Content-Type: application/json' "
                    "-H 'Authorization: Bearer %s' "
                    "-d '{\"messages\":[{\"type\":\"text\",\"text\":\"誰かが対戦相手を探しています。ゲームを起動して遊んでみましょう。https://github.com/eeic-software1-2025/assignment-week7-2-utyujinn\"}]}' "
                    "> /dev/null 2>&1 &",
                    access_token);
                system(command);
            }
        }
        if(log_use)printf("Client added as %s. Total clients: %d\n",
               clients[client_count-1].name, client_count);
    }
    pthread_mutex_unlock(&client_mutex);
}

static void remove_client(struct lws *wsi) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].wsi == wsi) {
            if(log_use)printf("%s disconnected\n", clients[i].name);
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;

            // クライアントが切断したらMENU状態にリセット
            pthread_mutex_lock(&scene_mutex);
            server_scene.state = STATE_MENU;
            server_scene.timer = 0;
            server_scene.timer2 = 0;
            server_scene.flag = 0;
            init_world(&server_scene.world);
            if(log_use)printf("Server reset to MENU state due to client disconnect\n");
            pthread_mutex_unlock(&scene_mutex);

            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

static int get_player_id(struct lws *wsi) {
    pthread_mutex_lock(&client_mutex);
    int player_id = 0;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].wsi == wsi) {
            player_id = clients[i].player_id;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
    return player_id;
}

// Scene全体をブロードキャスト
static void broadcast_scene(void) {
    unsigned char buf[LWS_PRE + sizeof(Scene)];

    pthread_mutex_lock(&scene_mutex);
    memcpy(&buf[LWS_PRE], &server_scene, sizeof(Scene));
    pthread_mutex_unlock(&scene_mutex);

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].wsi != NULL) {
            int result = lws_write(clients[i].wsi, &buf[LWS_PRE],
                                  sizeof(Scene), LWS_WRITE_BINARY);
            if (result < 0) {
                if(log_use)printf("Failed to send to %s\n", clients[i].name);
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// コールバック関数
static int callback(struct lws *wsi, enum lws_callback_reasons reason,
                   void *user, void *in, size_t len) {
    switch(reason) {
        case LWS_CALLBACK_ESTABLISHED:
            if(log_use)printf("Connection established\n");
            add_client(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
            // クライアントから入力を受信
            if (len > 0) {
                char msg[256];
                if (len < sizeof(msg)) {
                    memcpy(msg, in, len);
                    msg[len] = '\0';

                    // INITメッセージの処理: "INIT:ユーザー名:winCount"
                    if (strncmp(msg, "INIT:", 5) == 0) {
                        char client_name[64];
                        int win_count = 0;
                        if (sscanf(msg, "INIT:%63[^:]:%d", client_name, &win_count) == 2) {
                            pthread_mutex_lock(&client_mutex);
                            for (int i = 0; i < client_count; i++) {
                                if (clients[i].wsi == wsi && !clients[i].initialized) {
                                    // クライアント名とwinCountを保存
                                    strncpy(clients[i].name, client_name, sizeof(clients[i].name) - 1);
                                    clients[i].name[sizeof(clients[i].name) - 1] = '\0';
                                    clients[i].win_count = win_count;
                                    clients[i].initialized = 1;

                                    // シーンにwinCountを設定
                                    pthread_mutex_lock(&scene_mutex);
                                    if (clients[i].player_id == 1) {
                                        server_scene.world.player1.winCount = win_count;
                                    } else if (clients[i].player_id == 2) {
                                        server_scene.world.player2.winCount = win_count;
                                    }
                                    pthread_mutex_unlock(&scene_mutex);

                                    // クライアントにプレイヤー番号を送信
                                    unsigned char buf[LWS_PRE + 32];
                                    int msg_len = snprintf((char*)&buf[LWS_PRE], 32, "PLAYER:%d", clients[i].player_id);
                                    lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                                    lws_callback_on_writable(wsi);

                                    break;
                                }
                            }
                            pthread_mutex_unlock(&client_mutex);
                        }
                    } else {
                        // 通常の入力（1文字）
                        char input = msg[0];
                        int player_id = get_player_id(wsi);

                        //printf("Received '%c' from Player%d\n", input, player_id);

                        pthread_mutex_lock(&scene_mutex);
                        if (player_id == 1) {
                            server_scene.input_player1 = input;
                        } else if (player_id == 2) {
                            server_scene.input_player2 = input;
                        }
                        pthread_mutex_unlock(&scene_mutex);
                    }
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:
            if(log_use)printf("Connection closed\n");
            remove_client(wsi);
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "protocol",
        callback,
        0,
        1024,
    },
    { NULL, NULL, 0, 0 }
};

// ゲームループスレッド
static void* game_loop(void* arg) {
    (void)arg;
    GameState prev_state = STATE_TITLE;

    while(1) {
        client1_connected_time++;
        if(client1_connected_time == 300 && client_count == 1){
            bot_active = 1;
        }

        pthread_mutex_lock(&scene_mutex);

        if(bot_active && client_count == 1){
            server_scene.input_player2 = generate_bot_input(&server_scene);
        }

        if(server_scene.state==STATE_TITLE){
            process_frame_title(&server_scene,server_scene.input_player1,1); // タイトル入力はPlayer1基準で処理
        }
        else if(server_scene.state==STATE_MENU){
            process_frame_menu(&server_scene,server_scene.input_player1,server_scene.input_player2);
        }
        else if(server_scene.state==STATE_PLAY){
            process_frame(&server_scene,server_scene.input_player1,server_scene.input_player2);
        }
        else if(server_scene.state==STATE_RESULT){
            process_frame_result(&server_scene,server_scene.input_player1,server_scene.input_player2);
        }

        // PLAYからRESULTに遷移した時、勝者のwinCountを+1
        if(server_scene.state == STATE_RESULT && prev_state == STATE_PLAY){
            // player1が生きていてplayer2が死んでいる場合、player1の勝ち
            if(server_scene.world.player1.isAlive && !server_scene.world.player2.isAlive){
                if(server_scene.world.player1.winCount<999)server_scene.world.player1.winCount++;
            }
            // player2が生きていてplayer1が死んでいる場合、player2の勝ち
            else if(!server_scene.world.player1.isAlive && server_scene.world.player2.isAlive){
                if(server_scene.world.player2.winCount<999)server_scene.world.player2.winCount++;
            }
            // 両方死んでいる場合は引き分け
            else {
            }
        }

        prev_state = server_scene.state;

        // 入力をリセット（次フレームまで保持しない）
        server_scene.input_player1 = 0;
        server_scene.input_player2 = 0;

        //if(server_scene.timer%10==0)printf("Game State: %d, Timer: %d\n", server_scene.state, server_scene.timer);

        pthread_mutex_unlock(&scene_mutex);

        // Sceneをブロードキャスト
        broadcast_scene();

        usleep(33000);  // 30fps
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    for(int i=0;i<argc;i++){
        if(strcmp(argv[i],"--notify")==0){
            notify_use=1;
        }
        if(strcmp(argv[i],"--log")==0){
            log_use=1;
        }
    }
    struct lws_context_creation_info info;
    struct lws_context *context;
    pthread_t game_thread;

    // Scene初期化
    server_scene.state = STATE_MENU;
    server_scene.input_player1 = 0;
    server_scene.input_player2 = 0;
    server_scene.timer = 0;
    server_scene.timer2 = 0;
    server_scene.flag = 0;
    init_world(&server_scene.world);
    lws_set_log_level(0, NULL);

    //printf("Game Server Starting...\n");

    // WebSocketサーバー設定
    memset(&info, 0, sizeof(info));
    info.port = PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws_create_context failed\n");
        return 1;
    }

    //printf("WebSocket server started on port %d\n", PORT);

    // ゲームループスレッド開始
    pthread_create(&game_thread, NULL, game_loop, NULL);
    //printf("Game loop thread started\n");

    // WebSocketサービスループ
    while(1) {
        lws_service(context, 50);
    }

    lws_context_destroy(context);
    return 0;
}
