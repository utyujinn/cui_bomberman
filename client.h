#ifndef CLIENT_H
#define CLIENT_H

#include "types.h"

// WebSocket接続を初期化してコンテキストを返す

void* init_websocket_client(const char* client_name, int win_count, const char* server_addr, int port);

// 1文字をサーバーに送信
// key: 送信する文字
// 戻り値: 成功時0、失敗時-1
int send_key_to_server(char key);

// WebSocketサービスを1回実行（ノンブロッキング）
// context: init_websocket_clientで取得したコンテキスト
void service_websocket(void* context);

// 接続を終了してリソースを解放
// context: init_websocket_clientで取得したコンテキスト
void cleanup_websocket(void* context);

// 接続状態を確認
// 戻り値: 接続中なら1、切断中なら0
int is_websocket_connected(void);

// クライアントが実行中かどうかを確認
// 戻り値: 実行中なら1、終了中なら0
int is_client_running(void);

// サーバーから受信したSceneを取得
// scene: 受信したSceneをコピーする先のポインタ
void get_received_scene(Scene *scene);

// 入力モードを設定
// mode: 0=ローカル(Title), 1=サーバー送信
void set_input_mode(int mode);

// ローカル入力を取得（タイトル画面用）
// 戻り値: 入力された文字（入力がなければ0）
char get_local_input(void);

// raw modeを有効化して入力スレッドを開始
void start_input_thread(void);

// プログラム終了時のクリーンアップ（入力スレッド停止を含む）
void shutdown_client(void);

// 自分のプレイヤー番号を取得
// 戻り値: プレイヤー番号（1 or 2）、未設定の場合は-1
int get_my_player_id(void);

#endif