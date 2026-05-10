#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)
// BMPファイルヘッダー（14バイト）
typedef struct {
    uint16_t bfType;      // "BM"
    uint32_t bfSize;      // ファイルサイズ
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // 画像データまでのオフセット
} BMPFileHeader;

// BMP情報ヘッダー（40バイト）
typedef struct {
    uint32_t biSize;          // ヘッダーサイズ
    int32_t  biWidth;         // 画像の幅
    int32_t  biHeight;        // 画像の高さ
    uint16_t biPlanes;        // プレーン数（常に1）
    uint16_t biBitCount;      // 1ピクセルあたりのビット数
    uint32_t biCompression;   // 圧縮形式
    uint32_t biSizeImage;     // 画像データサイズ
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

// BMPファイルを読み込んで配列に変換
int bmpToArray(const char* filename, const char* outputFile) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return 1;
    }

    // ファイルヘッダー読み込み
    BMPFileHeader fileHeader;
    fread(&fileHeader, sizeof(BMPFileHeader), 1, fp);

    if (fileHeader.bfType != 0x4D42) { // "BM"
        fprintf(stderr, "Error: Not a BMP file\n");
        fclose(fp);
        return 1;
    }

    // 情報ヘッダー読み込み
    BMPInfoHeader infoHeader;
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, fp);

    printf("BMP Info:\n");
    printf("  Size: %dx%d\n", infoHeader.biWidth, abs(infoHeader.biHeight));
    printf("  Bit depth: %d\n", infoHeader.biBitCount);

    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        fprintf(stderr, "Error: Only 24-bit and 32-bit BMP files are supported\n");
        fclose(fp);
        return 1;
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bottomUp = (infoHeader.biHeight > 0); // 正の場合はボトムアップ
    int bytesPerPixel = infoHeader.biBitCount / 8;

    // 行パディングを計算（4バイト境界）
    int rowSize = ((width * bytesPerPixel + 3) / 4) * 4;

    // 画像データの位置に移動
    fseek(fp, fileHeader.bfOffBits, SEEK_SET);

    // 画像データを格納する配列
    uint32_t** pixels = (uint32_t**)malloc(height * sizeof(uint32_t*));
    for (int i = 0; i < height; i++) {
        pixels[i] = (uint32_t*)malloc(width * sizeof(uint32_t));
    }

    // ピクセルデータ読み込み
    uint8_t* rowData = (uint8_t*)malloc(rowSize);

    for (int y = 0; y < height; y++) {
        int targetY = bottomUp ? (height - 1 - y) : y;

        fread(rowData, 1, rowSize, fp);

        for (int x = 0; x < width; x++) {
            int offset = x * bytesPerPixel;
            uint8_t b = rowData[offset];
            uint8_t g = rowData[offset + 1];
            uint8_t r = rowData[offset + 2];

            pixels[targetY][x] = (r << 16) | (g << 8) | b;
        }
    }

    free(rowData);
    fclose(fp);

    // 配列として出力
    FILE* outFp = NULL;
    if (outputFile) {
        outFp = fopen(outputFile, "w");
        if (!outFp) {
            fprintf(stderr, "Warning: Cannot open output file %s, using stdout\n", outputFile);
            outFp = stdout;
        }
    } else {
        outFp = stdout;
    }

    fprintf(outFp, "// Generated from %s\n", filename);
    fprintf(outFp, "// Size: %dx%d\n", width, height);
    fprintf(outFp, "int texture[%d][%d] = {\n", height, width);

    for (int y = 0; y < height; y++) {
        fprintf(outFp, "    {");
        for (int x = 0; x < width; x++) {
            fprintf(outFp, "0x%06X", pixels[y][x]);
            if (x < width - 1) {
                fprintf(outFp, ", ");
            }
        }
        fprintf(outFp, "}");
        if (y < height - 1) {
            fprintf(outFp, ",");
        }
        fprintf(outFp, "\n");
    }

    fprintf(outFp, "};\n");

    if (outFp != stdout) {
        fclose(outFp);
        printf("\nArray saved to %s\n", outputFile);
    }

    // メモリ解放
    for (int i = 0; i < height; i++) {
        free(pixels[i]);
    }
    free(pixels);

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input.bmp> [output.txt]\n", argv[0]);
        printf("\n");
        printf("Convert BMP image to C array format\n");
        printf("\n");
        printf("Examples:\n");
        printf("  %s hoge.bmp                # Output to stdout\n", argv[0]);
        printf("  %s hoge.bmp output.txt     # Save to output.txt\n", argv[0]);
        printf("  %s sprite.bmp > sprite.h   # Redirect to sprite.h\n", argv[0]);
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = (argc >= 3) ? argv[2] : NULL;

    return bmpToArray(inputFile, outputFile);
}
