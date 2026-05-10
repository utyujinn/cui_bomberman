#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "graphics.h"
#include "title.h"
#include "graphics_title.h"
#include "graphics_menu.h"
#include "graphics_game.h"
#include "graphics_result.h"
#include "graphics_help.h"
#include "help.h"
#include "types.h"
#include "client.h"
#include "gamedata.h"
#include "config.h"

int main(int argc, char *argv[]){
    system("clear");
    Scene scene = {.state=STATE_TITLE, .timer=0, .gamemode=0, .connection_failed=0, .world.player1.name=0, .world.player2.name=0};

    // 入力スレッドの開始ローカル入力モード
    start_input_thread();
    set_input_mode(0);

    // オンラインモードの設定
    const char* client_name = (argc > 1) ? argv[1] : "User";

    // gamedata.txtからwinCountを読み込む

    char server_addr[256];
    int port = 8000;
    readconfig(".env", server_addr, &port);

    // WebSocketコンテキスト（タイトル画面後に初期化）
    void* ws_context = NULL;
    GameState prev_state = STATE_TITLE;
    pid_t server_pid = 0;  // ローカルサーバーのPID
    char server_started = 0;

    while(1){
        if(scene.state == STATE_TITLE){
            char c = get_local_input();

            // スペースキーが押されたらローカルサーバーを事前起動
            if((c == ' ' || c == '\n') && scene.gamemode == 0 && !server_started){
                server_pid = fork();
                if(server_pid == 0){
                    // 子プロセス：サーバーを実行
                    execl("./server", "server", NULL);
                    exit(1);  // execl失敗時
                }
                server_started = 1;
            }

            process_frame_title(&scene, c, 1);

            // タイトル画面でqを押した場合はゲーム終了
            if(scene.flag == 2){
                break;
            }

            // 接続失敗からの復帰
            if(scene.connection_failed){
                scene.connection_failed = 0;
                scene.state = STATE_TITLE;
                scene.flag = 0;
                scene.gamemode = 0;
                server_started = 0;
                if(server_pid > 0){
                    kill(server_pid, SIGTERM);
                    server_pid = 0;
                }
            }
        }

        if(scene.state == STATE_HELP){
            char c = get_local_input();
            process_frame_help(&scene, c, 0);
        }

        if(scene.state != STATE_HELP && scene.state != STATE_TITLE && prev_state == STATE_TITLE && ws_context == NULL){
            
            int win_count = load_win_count(client_name);
            
            if(scene.gamemode == 0){
                ws_context = init_websocket_client(client_name, win_count, "localhost", 8000);
            }
            else if(scene.gamemode == 1){
                ws_context = init_websocket_client(client_name, win_count, server_addr, port);
            }

            if (!ws_context) {
                fprintf(stderr, "Failed to connect to WebSocket server\n");
                scene.connection_failed = 1;
                scene.state = STATE_TITLE;
                continue;
            }
        }
    
        if(ws_context){
            service_websocket(ws_context);
            get_received_scene(&scene);
        }
    
        if(scene.state == STATE_TITLE){
            drawBlack();
            drawTitleBuff(&scene);
        }
        if(scene.state == STATE_HELP){
            drawBlack();
            drawHelpBuff(&scene);
        }
        else if(scene.state == STATE_MENU){
            drawBlack();
            drawMenuBuff(&scene);
        }
        else if(scene.state == STATE_PLAY){
            drawGameBuff(&scene);
        }
        else if(scene.state == STATE_RESULT){
            drawResultBuff(&scene);
        }

        // リザルト画面に入った時にwinCountを保存
        if(scene.state == STATE_RESULT && prev_state != STATE_RESULT){
            // サーバーから受信したsceneのwinCountを保存
            int my_id = get_my_player_id();
            int my_win_count;

            if(my_id == 1){
                my_win_count = scene.world.player1.winCount;
            } else if(my_id == 2){
                my_win_count = scene.world.player2.winCount;
            } else {
                // プレイヤー番号が不明な場合は保存しない
                my_win_count = -1;
            }

            if(my_win_count >= 0){
                save_win_count(client_name, my_win_count);
            }
        }

        // リザルト画面からタイトルに戻る際、サーバーから離脱
        if(scene.state == STATE_TITLE && prev_state == STATE_RESULT && ws_context != NULL){
            //printf("Disconnecting from server and returning to title...\n");
            cleanup_websocket(ws_context);
            ws_context = NULL;
            set_input_mode(0);  // ローカルモードに戻す

            // ローカルサーバーを起動していた場合は停止
            if(server_pid > 0){
                //printf("Stopping local server (PID: %d)...\n", server_pid);
                kill(server_pid, SIGTERM);
                server_pid = 0;
            }

            scene.flag = 0;  // フラグをリセット
            scene.gamemode = 0;  // ゲームモードもリセット
            scene.connection_failed = 0;  // 接続失敗フラグもリセット
            server_started = 0;  // サーバー起動フラグもリセット
        }

        printScreen();
        usleep(33000);
        prev_state = scene.state;
    }

    if(ws_context) cleanup_websocket(ws_context);

    // ローカルサーバーを起動していた場合は停止
    if(server_pid > 0){
        //printf("Stopping local server (PID: %d)...\n", server_pid);
        kill(server_pid, SIGTERM);
    }

    // 入力スレッドを停止してプログラム終了
    shutdown_client();

    //printf("Game Over!\n");
    return 0;
}
