#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define MAX_CANVAS_WIDTH 128
#define MAX_CANVAS_HEIGHT 128

// キャンバスサイズ（可変）
int canvasWidth = 8;
int canvasHeight = 8;

// カーソル位置とキャンバス
int canvas[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH]; // RGB値を格納
int cursorX = 0, cursorY = 0;
int currentColor = 0xFFFFFF; // 初期色は白

// カラーパレット
int palette[] = {
    0x000000, // 黒
    0xFFFFFF, // 白
    0xFF0000, // 赤
    0x00FF00, // 緑
    0x0000FF, // 青
    0xFFFF00, // 黄
    0xFF00FF, // マゼンタ
    0x00FFFF, // シアン
    0x808080, // グレー
    0xA00000, // ダークレッド
};
int paletteIndex = 1; // 初期は白

// カラーピッカーモード用
int colorPickerMode = 0; // 0=通常, 1=RGBバー調整
int selectedChannel = 0; // 0=R, 1=G, 2=B

// 透過モード用
int transparencyMode = 0; // 0=通常, 1=透過

// kbhit関数（main.cから流用）
int kbhit(){
  struct termios oldt, newt;
  int c;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  c = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if(c!=EOF){ungetc(c,stdin);return 1;}
  return 0;
}

// キャンバスを初期化
void initCanvas(){
    for(int y = 0; y < MAX_CANVAS_HEIGHT; y++){
        for(int x = 0; x < MAX_CANVAS_WIDTH; x++){
            canvas[y][x] = 0x202020; // 初期は暗いグレー
        }
    }
}

// color.txtから配列を読み込む
int loadFromFile(const char* filename){
    FILE* fp = fopen(filename, "r");
    if(!fp){
        printf("Failed to open %s\n", filename);
        return 0;
    }

    char line[1024];
    int y = 0;
    int readingArray = 0;
    int detectedWidth = 0, detectedHeight = 0;

    while(fgets(line, sizeof(line), fp)){
        // "int texture[" で始まる行を探す
        if(strstr(line, "int texture[") || strstr(line, "int ") && strchr(line, '[')){
            // サイズを検出
            int h, w;
            if(sscanf(line, "int texture[%d][%d]", &h, &w) == 2 ||
               sscanf(line, "int %*[^[][%d][%d]", &h, &w) == 2){
                detectedHeight = h;
                detectedWidth = w;
                if(detectedHeight > MAX_CANVAS_HEIGHT) detectedHeight = MAX_CANVAS_HEIGHT;
                if(detectedWidth > MAX_CANVAS_WIDTH) detectedWidth = MAX_CANVAS_WIDTH;
            }
            readingArray = 1;
            y = 0;
            continue;
        }

        if(readingArray && strchr(line, '{')){
            // 行から16進数を抽出
            char* ptr = line;
            int x = 0;
            while(*ptr && x < MAX_CANVAS_WIDTH){
                if(strncmp(ptr, "0x", 2) == 0 || strncmp(ptr, "0X", 2) == 0){
                    int color;
                    if(sscanf(ptr, "%x", &color) == 1){
                        if(y < MAX_CANVAS_HEIGHT && x < MAX_CANVAS_WIDTH){
                            canvas[y][x] = color; // 透過フラグも含めて保存
                        }
                        x++;
                    }
                    ptr += 2;
                } else {
                    ptr++;
                }
            }
            if(x > 0){
                y++;
                if(y >= detectedHeight) break;
            }
        }

        if(strstr(line, "};")){
            readingArray = 0;
            break;
        }
    }

    fclose(fp);

    if(detectedWidth > 0 && detectedHeight > 0){
        canvasWidth = detectedWidth;
        canvasHeight = detectedHeight;
        printf("Loaded %dx%d image from %s\n", canvasWidth, canvasHeight, filename);
        return 1;
    }

    return 0;
}

// RGBカラーバーを表示
void printColorPicker(){
    int r = (currentColor >> 16) & 0xFF;
    int g = (currentColor >> 8) & 0xFF;
    int b = currentColor & 0xFF;

    printf("\n=== Color Picker ===\n");
    printf("Use W/S to select channel, A/D to adjust value, ENTER to finish\n\n");

    // Rチャンネル
    printf("%s R: %3d [", selectedChannel == 0 ? ">" : " ", r);
    for(int i = 0; i < 32; i++){
        int val = (i * 255) / 31;
        if(val <= r){
            printf("\033[38;2;255;0;0m█\033[0m");
        } else {
            printf("░");
        }
    }
    printf("]\n");

    // Gチャンネル
    printf("%s G: %3d [", selectedChannel == 1 ? ">" : " ", g);
    for(int i = 0; i < 32; i++){
        int val = (i * 255) / 31;
        if(val <= g){
            printf("\033[38;2;0;255;0m█\033[0m");
        } else {
            printf("░");
        }
    }
    printf("]\n");

    // Bチャンネル
    printf("%s B: %3d [", selectedChannel == 2 ? ">" : " ", b);
    for(int i = 0; i < 32; i++){
        int val = (i * 255) / 31;
        if(val <= b){
            printf("\033[38;2;0;0;255m█\033[0m");
        } else {
            printf("░");
        }
    }
    printf("]\n");

    printf("\nCurrent color: \033[48;2;%d;%d;%dm      \033[0m #%02X%02X%02X\n",
           r, g, b, r, g, b);
}

// ▀を使ってキャンバスを描画（2行で1文字）
void printCanvas(){
    printf("\033[H\033[J"); // カーソルをホームに移動してクリア

    printf("=== Bitmap Editor (%dx%d) ===\n", canvasWidth, canvasHeight);
    printf("Controls: WASD=move, SPACE=paint, I=eyedropper, C=palette, R=RGB picker, H=hex input, T=transparency, L=load, E=export, Q=quit\n");
    printf("Current color: \033[48;2;%d;%d;%dm   \033[0m #%02X%02X%02X %s\n\n",
           (currentColor >> 16) & 0xFF,
           (currentColor >> 8) & 0xFF,
           currentColor & 0xFF,
           (currentColor >> 16) & 0xFF,
           (currentColor >> 8) & 0xFF,
           currentColor & 0xFF,
           transparencyMode ? "[TRANSPARENT]" : "");

    // canvasHeight/2 行で表示（各行が2ピクセルの高さ）
    for(int row = 0; row < (canvasHeight + 1) / 2; row++){
        for(int col = 0; col < canvasWidth; col++){
            int topY = row * 2;
            int bottomY = row * 2 + 1;

            // 範囲チェック
            if(topY >= canvasHeight) break;

            int topColor = canvas[topY][col];
            int bottomColor = (bottomY < canvasHeight) ? canvas[bottomY][col] : 0x202020;

            // 透過フラグをチェック
            int topIsTransparent = (topColor & 0x01000000) != 0;
            int bottomIsTransparent = (bottomColor & 0x01000000) != 0;

            // チェッカーボード模様の背景色（位置に応じて交互に）
            int checkerDark = 0x404040;
            int checkerLight = 0x606060;
            int topChecker = ((topY + col) % 2 == 0) ? checkerDark : checkerLight;
            int bottomChecker = ((bottomY + col) % 2 == 0) ? checkerDark : checkerLight;

            // 透過の場合はチェッカーボード模様、そうでなければ実際の色
            int displayTopColor = topIsTransparent ? topChecker : topColor;
            int displayBottomColor = bottomIsTransparent ? bottomChecker : bottomColor;

            int topR = (displayTopColor >> 16) & 0xFF;
            int topG = (displayTopColor >> 8) & 0xFF;
            int topB = displayTopColor & 0xFF;

            int bottomR = (displayBottomColor >> 16) & 0xFF;
            int bottomG = (displayBottomColor >> 8) & 0xFF;
            int bottomB = displayBottomColor & 0xFF;

            // カーソル位置を強調表示
            if(col == cursorX && topY == cursorY){
                // カーソルが上半分にある場合、上を黄色枠で囲む
                printf("\033[38;2;255;255;0m\033[48;2;%d;%d;%dm▀\033[0m",
                       bottomR, bottomG, bottomB);
            } else if(col == cursorX && bottomY == cursorY && bottomY < canvasHeight){
                // カーソルが下半分にある場合、背景を黄色枠で囲む
                printf("\033[38;2;%d;%d;%dm\033[48;2;255;255;0m▀\033[0m",
                       topR, topG, topB);
            } else {
                // 文字色=上のピクセル、背景色=下のピクセル
                printf("\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀\033[0m",
                       topR, topG, topB,
                       bottomR, bottomG, bottomB);
            }
        }
        printf("\n");
    }

    printf("\nCursor: (%d, %d)\n", cursorX, cursorY);
}

// RGBカラーピッカーの入力処理
void handleColorPickerInput(char c){
    int r = (currentColor >> 16) & 0xFF;
    int g = (currentColor >> 8) & 0xFF;
    int b = currentColor & 0xFF;

    switch(c){
        case 'w': // チャンネル選択を上へ
            if(selectedChannel > 0) selectedChannel--;
            break;
        case 's': // チャンネル選択を下へ
            if(selectedChannel < 2) selectedChannel++;
            break;
        case 'a': // 値を減らす
            if(selectedChannel == 0 && r > 0) r -= 5;
            else if(selectedChannel == 1 && g > 0) g -= 5;
            else if(selectedChannel == 2 && b > 0) b -= 5;
            currentColor = (r << 16) | (g << 8) | b;
            break;
        case 'd': // 値を増やす
            if(selectedChannel == 0 && r < 255) r = (r + 5 > 255) ? 255 : r + 5;
            else if(selectedChannel == 1 && g < 255) g = (g + 5 > 255) ? 255 : g + 5;
            else if(selectedChannel == 2 && b < 255) b = (b + 5 > 255) ? 255 : b + 5;
            currentColor = (r << 16) | (g << 8) | b;
            break;
        case '\n': // Enterで終了
        case '\r':
            colorPickerMode = 0;
            break;
    }
}

// 16進数で色入力
void inputHexColor(){
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("\n\nEnter hex color (e.g., FF00AA): ");
    char hexInput[7];
    if(fgets(hexInput, sizeof(hexInput), stdin) != NULL){
        int color;
        if(sscanf(hexInput, "%x", &color) == 1){
            currentColor = color & 0xFFFFFF;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// 入力処理
void handleInput(char c){
    switch(c){
        case 'w': // 上
            if(cursorY > 0) cursorY--;
            break;
        case 's': // 下
            if(cursorY < canvasHeight - 1) cursorY++;
            break;
        case 'a': // 左
            if(cursorX > 0) cursorX--;
            break;
        case 'd': // 右
            if(cursorX < canvasWidth - 1) cursorX++;
            break;
        case ' ': // スペース：ペイント
            if(transparencyMode){
                canvas[cursorY][cursorX] = currentColor | 0x01000000; // 透過フラグを設定
            } else {
                canvas[cursorY][cursorX] = currentColor & 0x00FFFFFF; // 透過フラグをクリア
            }
            break;
        case 'i': // スポイト：カーソル位置の色を取得
            {
                int pickedColor = canvas[cursorY][cursorX];
                // 透過フラグをチェック
                if(pickedColor & 0x01000000){
                    transparencyMode = 1;
                    currentColor = pickedColor & 0x00FFFFFF;
                } else {
                    transparencyMode = 0;
                    currentColor = pickedColor & 0x00FFFFFF;
                }
            }
            break;
        case 'c': // パレットから色変更
            paletteIndex = (paletteIndex + 1) % (sizeof(palette) / sizeof(palette[0]));
            currentColor = palette[paletteIndex];
            break;
        case 'r': // RGBカラーピッカーモード
            colorPickerMode = 1;
            selectedChannel = 0;
            break;
        case 'h': // 16進数入力
            inputHexColor();
            break;
        case 't': // 透過モード切り替え
            transparencyMode = !transparencyMode;
            break;
        case 'l': // ファイルから読み込み
            {
                struct termios oldt, newt;
                tcgetattr(STDIN_FILENO, &oldt);
                newt = oldt;
                newt.c_lflag |= (ICANON | ECHO);
                tcsetattr(STDIN_FILENO, TCSANOW, &newt);

                printf("\n\nEnter filename to load (e.g., color.txt): ");
                char filename[256];
                if(fgets(filename, sizeof(filename), stdin) != NULL){
                    // 改行を削除
                    filename[strcspn(filename, "\n")] = 0;
                    loadFromFile(filename);
                    printf("Press any key to continue...");
                    getchar();
                }

                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            }
            break;
        case 'e': // エクスポート
            printf("\n\nExported array:\n");
            printf("int texture[%d][%d] = {\n", canvasHeight, canvasWidth);
            for(int y = 0; y < canvasHeight; y++){
                printf("    {");
                for(int x = 0; x < canvasWidth; x++){
                    // 透過フラグを含めた32ビット値を出力
                    if(canvas[y][x] & 0x01000000){
                        printf("0x%08X", canvas[y][x]);
                    } else {
                        printf("0x%06X", canvas[y][x] & 0x00FFFFFF);
                    }
                    if(x < canvasWidth - 1) printf(", ");
                }
                printf("}");
                if(y < canvasHeight - 1) printf(",");
                printf("\n");
            }
            printf("};\n");
            printf("\nPress any key to continue...");
            getchar();
            break;
    }
}

int main(int argc, char* argv[]){
    // コマンドライン引数でサイズを指定可能
    if(argc >= 3){
        int w = atoi(argv[1]);
        int h = atoi(argv[2]);
        if(w > 0 && w <= MAX_CANVAS_WIDTH && h > 0 && h <= MAX_CANVAS_HEIGHT){
            canvasWidth = w;
            canvasHeight = h;
        } else {
            printf("Invalid size. Max: %dx%d\n", MAX_CANVAS_WIDTH, MAX_CANVAS_HEIGHT);
            return 1;
        }
    }

    // ファイル名が指定されていれば読み込む
    if(argc >= 4){
        initCanvas();
        loadFromFile(argv[3]);
    } else {
        initCanvas();
    }

    printf("\033[2J"); // 画面クリア
    printf("Bitmap Editor - Canvas size: %dx%d\n", canvasWidth, canvasHeight);
    printf("Usage: %s [width] [height] [file.txt]\n", argv[0]);
    sleep(2);

    while(1){
        if(colorPickerMode){
            // カラーピッカーモード
            printf("\033[H\033[J");
            printColorPicker();

            char c = 0;
            if(kbhit()){
                c = getchar();
                if(c == 'q'){
                    colorPickerMode = 0; // qでキャンセル
                } else {
                    handleColorPickerInput(c);
                }
            }
        } else {
            // 通常のキャンバス編集モード
            printCanvas();

            char c = 0;
            if(kbhit()){
                c = getchar();
                if(c == 'q') break; // q で終了
                handleInput(c);
            }
        }

        usleep(50000); // 50ms待機
    }

    printf("\033[0m\n"); // 色をリセット
    return 0;
}
