#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
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
    int ws_first_recv = 0;  // DBG: ws_context作成直後のget_received_scene用フラグ

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
                fprintf(stderr, "[DBG] 新サーバー起動: PID=%d\n", server_pid);
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

            fprintf(stderr, "[DBG] WebSocket接続開始: gamemode=%d, scene.state=%d\n", scene.gamemode, scene.state);
            if(scene.gamemode == 0){
                ws_context = init_websocket_client(client_name, win_count, "localhost", 8000);
            }
            else if(scene.gamemode == 1){
                ws_context = init_websocket_client(client_name, win_count, server_addr, port);
            }

            if (!ws_context) {
                fprintf(stderr, "[DBG] WebSocket接続失敗: ws_context=NULL\n");
                scene.connection_failed = 1;
                scene.state = STATE_TITLE;
                continue;
            }
            fprintf(stderr, "[DBG] WebSocket接続開始成功: ws_context=%p\n", ws_context);
            ws_first_recv = 1;
        }
    
        if(ws_context){
            service_websocket(ws_context);
            get_received_scene(&scene);
            if(ws_first_recv){
                fprintf(stderr, "[DBG] 接続直後の受信scene: state=%d, flag=%d, timer2=%d, connected=%d\n",
                        scene.state, scene.flag, scene.timer2, is_websocket_connected());
                ws_first_recv = 0;
            }
            // 接続エラー時はタイトルに戻る
            if(is_connection_error()){
                fprintf(stderr, "[DBG] 接続エラー検知: タイトルに戻ります\n");
                cleanup_websocket(ws_context);
                ws_context = NULL;
                set_input_mode(0);
                scene.connection_failed = 1;
                scene.state = STATE_TITLE;
                scene.flag = 0;
                if(server_pid > 0){
                    kill(server_pid, SIGTERM);
                    waitpid(server_pid, NULL, 0);
                    server_pid = 0;
                }
                server_started = 0;
                continue;
            }
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
            fprintf(stderr, "[DBG] RESULT->TITLE クリーンアップ開始: server_pid=%d\n", server_pid);
            cleanup_websocket(ws_context);
            ws_context = NULL;
            set_input_mode(0);  // ローカルモードに戻す

            // ローカルサーバーを起動していた場合は停止
            if(server_pid > 0){
                fprintf(stderr, "[DBG] サーバー停止: kill(PID=%d) waitpidあり\n", server_pid);
                kill(server_pid, SIGTERM);
                waitpid(server_pid, NULL, 0);
                server_pid = 0;
            }

            scene.flag = 0;  // フラグをリセット
            scene.gamemode = 0;  // ゲームモードもリセット
            scene.connection_failed = 0;  // 接続失敗フラグもリセット
            server_started = 0;  // サーバー起動フラグもリセット
            fprintf(stderr, "[DBG] クリーンアップ完了\n");
        }

        if(scene.state != prev_state){
            fprintf(stderr, "[DBG] 状態遷移: %d -> %d (flag=%d, timer2=%d, ws=%p)\n",
                    prev_state, scene.state, scene.flag, scene.timer2, ws_context);
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
