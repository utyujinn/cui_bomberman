#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include "client.h"
#include "types.h"

// 十字キー軸・デッドゾーン (両コントローラ共通)
#define JOY_AXIS_DX   6
#define JOY_AXIS_DY   7
#define JOY_DEADZONE  16384

// コントローラ個別のボタン設定
struct joy_config {
    int btn_bomb;   // 右ボタン → ' ' (爆弾 / リザルト→キャラ選択)
    int btn_quit;   // 下ボタン → 'q' (リザルト→タイトル / タイトル→終了)
    int btn_l;      // ステージ左 → 'j'
    int btn_r;      // ステージ右 → 'l'
};

struct joy_thread_arg {
    int fd;
    char path[32];
    struct joy_config cfg;
};

#define MAX_JOYSTICKS 4

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

struct session_data {
    char client_name[64];
    int win_count;
    struct lws *wsi;
    int connected;
    int connection_error;  // 接続エラー発生フラグ
};

static struct session_data global_session;
static struct termios orig_termios;
static volatile int running = 1;
static pthread_t input_tid;
static Scene received_scene;
static pthread_mutex_t scene_mutex = PTHREAD_MUTEX_INITIALIZER;
static int my_player_id = -1;  // 自分のプレイヤー番号 (1 or 2, -1は未設定)

// 入力モード管理
static volatile int input_mode = 0;  // 0=ローカル(Title), 1=サーバー送信
static char last_local_input = 0;    // ローカルモード用の入力バッファ
static pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t joy_tid[MAX_JOYSTICKS];
static int joy_thread_count = 0;

// 文字をEnterなしで取得するための端末設定
static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// libwebsocketsのコールバック関数
static int callback_client(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len) {
    struct session_data *session = (struct session_data *)user;
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "[DBG] 接続確立: %s (running=%d)\n", session->client_name, running);
            session->connected = 1;
            session->wsi = wsi;

            // 接続確立時にclient_nameとwinCountを送信
            {
                char init_msg[128];
                snprintf(init_msg, sizeof(init_msg), "INIT:%s:%d", session->client_name, session->win_count);
                size_t msg_len = strlen(init_msg);
                unsigned char buf[LWS_PRE + 128];
                memcpy(&buf[LWS_PRE], init_msg, msg_len);
                lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            // バイナリデータとしてSceneを受信
            if (len == sizeof(Scene)) {
                pthread_mutex_lock(&scene_mutex);
                memcpy(&received_scene, in, sizeof(Scene));
                pthread_mutex_unlock(&scene_mutex);
            } else if (len < 20) {
                // 短いメッセージはテキストとして処理（プレイヤー番号など）
                char msg[32];
                if (len < sizeof(msg)) {
                    memcpy(msg, in, len);
                    msg[len] = '\0';

                    // "PLAYER:1" または "PLAYER:2" の形式
                    if (strncmp(msg, "PLAYER:", 7) == 0) {
                        my_player_id = atoi(msg + 7);
                        //printf("[%s] Assigned as player %d\n", session->client_name, my_player_id);
                    }
                }
            }
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "[DBG] 接続エラー: %s (running=%d)\n", session->client_name, running);
            session->connected = 0;
            session->connection_error = 1;
            // running = 0 は shutdown_client() のみで呼ぶ。ここでは入力スレッドを止めない
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "[DBG] 接続クローズ: %s (running=%d)\n", session->client_name, running);
            session->connected = 0;
            // running = 0 は shutdown_client() のみで呼ぶ。ここでは入力スレッドを止めない
            break;

        default:
            break;
    }
    
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "protocol",              // server-side protocol name
        callback_client,
        sizeof(struct session_data),
        1024,
    },
    { NULL, NULL, 0, 0 }
};

// 入力の送信ヘルパー
static void send_message(const char* message) {
    if (global_session.connected && global_session.wsi) {
        unsigned char buf[LWS_PRE + 1024];
        int msg_len = strlen(message);
        memcpy(&buf[LWS_PRE], message, msg_len);

        int result = lws_write(global_session.wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
        if (result < 0) {
            //printf("Failed to send message\n");
        } else {
            //printf("[%s] Sent: %s\n", global_session.client_name, message);
        }
    }
}

int send_key_to_server(char key) {
    if (global_session.connected && global_session.wsi) {
        unsigned char buf[LWS_PRE + 2];
        buf[LWS_PRE] = key;

        int result = lws_write(global_session.wsi, &buf[LWS_PRE], 1, LWS_WRITE_TEXT);
        if (result < 0) {
            //printf("Failed to send key\n");
            return -1;
        } else {
            //printf("[%s] Sent: %c\n", global_session.client_name, key);
            return 0;
        }
    }
    return -1;
}

// 文字を現在のモードに応じてバッファ or サーバーへ送る共通ヘルパー
static void inject_char(char c) {
    pthread_mutex_lock(&input_mutex);
    if (input_mode == 0) {
        last_local_input = c;
    } else {
        char msg[8];
        snprintf(msg, sizeof(msg), "%c", c);
        send_message(msg);
    }
    pthread_mutex_unlock(&input_mutex);
}

// 入力スレッド関数(Enterなしで入力を処理できる)
static void* input_thread(void* arg) {
    (void)arg;
    while (running) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n > 0) {
            inject_char(c);
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            running = 0;
            break;
        }
    }
    return NULL;
}

// コントローラ名からボタン設定を生成
static struct joy_config make_joy_config(int fd) {
    char name[128] = "(unknown)";
    ioctl(fd, JSIOCGNAME(sizeof(name)), name);
    fprintf(stderr, "[DBG] コントローラ名: %s\n", name);

    struct joy_config cfg;
    cfg.btn_bomb = 1;   // ○/B: 右ボタン (両コントローラ共通)
    cfg.btn_l    = 4;
    cfg.btn_r    = 5;

    if (strstr(name, "DualShock") || strstr(name, "Wireless Controller") ||
        strstr(name, "Sony") || strstr(name, "PLAYSTATION")) {
        cfg.btn_quit = 0;   // DS4: ×(下) = button 0
    } else {
        cfg.btn_quit = 1;   // HORICON等: B(下) = button 1
        cfg.btn_bomb = 2;   // HORICON等: A(右) = button 2
    }
    return cfg;
}

// コントローラ入力スレッド (切断→再接続を自動検知)
static void* joystick_thread(void* arg) {
    struct joy_thread_arg* jarg = (struct joy_thread_arg*)arg;
    int fd = jarg->fd;
    struct joy_config cfg = jarg->cfg;
    char path[32];
    strncpy(path, jarg->path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    free(jarg);

    int axis_dx = 0, axis_dy = 0;
    int send_tick = 0;

    while (running) {
        // 切断中: 500ms おきに再接続を試みる
        if (fd < 0) {
            usleep(500000);
            fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                fprintf(stderr, "[DBG] コントローラ再接続: %s\n", path);
                axis_dx = axis_dy = send_tick = 0;
            }
            continue;
        }

        // イベント読み取り
        errno = 0;
        struct js_event ev;
        ssize_t n;
        while ((n = read(fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
            int type = ev.type & ~JS_EVENT_INIT;
            if (type == JS_EVENT_BUTTON && ev.value == 1) {
                char c = 0;
                if      (ev.number == cfg.btn_bomb) c = ' ';
                else if (ev.number == cfg.btn_quit) c = 'q';
                else if (ev.number == cfg.btn_l)    c = 'j';
                else if (ev.number == cfg.btn_r)    c = 'l';
                if (c) inject_char(c);
            } else if (type == JS_EVENT_AXIS) {
                if (ev.number == JOY_AXIS_DX) axis_dx = ev.value;
                else if (ev.number == JOY_AXIS_DY) axis_dy = ev.value;
            }
        }

        // 切断検知 (EAGAIN以外のエラー)
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "[DBG] コントローラ切断: %s\n", path);
            close(fd);
            fd = -1;
            axis_dx = axis_dy = 0;
            continue;
        }

        // 方向入力を ~10fps (3 * 33ms) で送信
        if (++send_tick >= 3) {
            send_tick = 0;
            char c = 0;
            int dx = axis_dx < 0 ? -axis_dx : axis_dx;
            int dy = axis_dy < 0 ? -axis_dy : axis_dy;
            if (dx >= JOY_DEADZONE || dy >= JOY_DEADZONE) {
                if (dx >= dy)
                    c = (axis_dx > 0) ? 'd' : 'a';
                else
                    c = (axis_dy > 0) ? 's' : 'w';
            }
            if (c) inject_char(c);
        }

        usleep(33000);
    }

    if (fd >= 0) close(fd);
    return NULL;
}

// WebSocket接続を初期化
void* init_websocket_client(const char* client_name, int win_count, const char* server_addr, int port) {
    lws_set_log_level(0, NULL);
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws_client_connect_info ccinfo;

    // クライアント名とwinCountを設定
    strncpy(global_session.client_name, client_name, sizeof(global_session.client_name) - 1);
    global_session.client_name[sizeof(global_session.client_name) - 1] = '\0';
    global_session.win_count = win_count;
    global_session.connected = 0;
    global_session.wsi = NULL;
    global_session.connection_error = 0;
    my_player_id = -1;  // プレイヤー番号をリセット

    // 前ゲームの古いsceneデータをリセット（flag/timer2が残ると描画が壊れる）
    pthread_mutex_lock(&scene_mutex);
    memset(&received_scene, 0, sizeof(received_scene));
    received_scene.state = STATE_MENU;
    pthread_mutex_unlock(&scene_mutex);

    // コンテキストを作成
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws_create_context failed\n");
        return NULL;
    }

    // 接続情報を設定
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = server_addr;
    ccinfo.port = port;
    ccinfo.path = "/";
    ccinfo.host = server_addr;
    ccinfo.origin = server_addr;
    ccinfo.protocol = "protocol";
    ccinfo.userdata = &global_session;

    if (!lws_client_connect_via_info(&ccinfo)) {
        fprintf(stderr, "Connection failed\n");
        lws_context_destroy(context);
        return NULL;
    }

    //printf("[%s] Connecting to WebSocket server at %s:%d...\n",global_session.client_name, server_addr, port);

    // 入力モードをサーバー送信に設定
    set_input_mode(1);

    return context;
}

// WebSocketサービスを1回実行
void service_websocket(void* context) {
    if (context) {
        lws_service((struct lws_context*)context, 50);
    }
}

// 接続を終了
void cleanup_websocket(void* context) {
    // WebSocketコンテキストのみクリーンアップ
    // 入力スレッドは停止しない（タイトルに戻った後も入力を受け付けるため）
    if (context) {
        lws_context_destroy((struct lws_context*)context);
    }
}

// プログラム終了時のクリーンアップ（入力スレッド停止を含む）
void shutdown_client(void) {
    running = 0;
    pthread_join(input_tid, NULL);
    for (int i = 0; i < joy_thread_count; i++)
        pthread_join(joy_tid[i], NULL);
    disable_raw_mode();
}

// 接続状態を確認
int is_websocket_connected(void) {
    return global_session.connected;
}

// クライアントが実行中かどうかを確認
int is_client_running(void) {
    return running;
}

// 受信したSceneを取得
void get_received_scene(Scene *scene) {
    pthread_mutex_lock(&scene_mutex);
    memcpy(scene, &received_scene, sizeof(Scene));
    pthread_mutex_unlock(&scene_mutex);
}

// 入力モードを設定
void set_input_mode(int mode) {
    pthread_mutex_lock(&input_mutex);
    input_mode = mode;
    pthread_mutex_unlock(&input_mutex);
}

// ローカル入力を取得（タイトル画面用）
char get_local_input(void) {
    pthread_mutex_lock(&input_mutex);
    char c = last_local_input;
    last_local_input = 0;  // 読み取ったらクリア
    pthread_mutex_unlock(&input_mutex);
    return c;
}

// raw modeを有効化して入力スレッドを開始
void start_input_thread(void) {
    enable_raw_mode();
    pthread_create(&input_tid, NULL, input_thread, NULL);

    // 接続されているコントローラをすべて起動
    const char* paths[] = {"/dev/input/js0", "/dev/input/js1", "/dev/input/js2", "/dev/input/js3"};
    joy_thread_count = 0;
    for (int i = 0; i < 4 && joy_thread_count < MAX_JOYSTICKS; i++) {
        int fd = open(paths[i], O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct joy_thread_arg* jarg = malloc(sizeof(struct joy_thread_arg));
        if (!jarg) { close(fd); continue; }
        jarg->fd  = fd;
        strncpy(jarg->path, paths[i], sizeof(jarg->path) - 1);
        jarg->path[sizeof(jarg->path) - 1] = '\0';
        jarg->cfg = make_joy_config(fd);

        if (pthread_create(&joy_tid[joy_thread_count], NULL, joystick_thread, jarg) == 0) {
            joy_thread_count++;
        } else {
            free(jarg);
            close(fd);
        }
    }
    if (joy_thread_count == 0)
        fprintf(stderr, "[DBG] コントローラ未検出 (/dev/input/js0-3)\n");
}

// 自分のプレイヤー番号を取得
int get_my_player_id(void) {
    return my_player_id;
}

// 接続エラーが発生したか確認（発生後は自動でリセット）
int is_connection_error(void) {
    if(global_session.connection_error){
        global_session.connection_error = 0;
        return 1;
    }
    return 0;
}
