# ボンバーマン

<img width="969" height="1034" alt="Image" src="https://github.com/user-attachments/assets/a292adce-1586-44a6-b1a1-a5d6305e79dc" />

## メンバー・おおまかな担当

- utyujinn
  - 全体構造の把握
  - リファクタリング、ファイル分割
  - 画面遷移処理
  - WebSocket通信
  - ボット作成
  - API通知機能作成
  - リザルト画面
  - ドット絵（背景・アイテム・キャラクター系の半分）
  
- AmatsuItadaki
  - 描画系の根幹関数
  - アイテム系・火などのゲームシステム処理
  - キャラクター・ステージ選択
  - ヘルプ画面
  - ドット絵（背景・アイテム・キャラクター系の半分）

## 概説

- 1マスを8x4文字サイズに拡大し、様々なグラフィックに対応しました。
- 描画は上ハーフブロック文字（▀）を使って実質8x8ドットを実現しています。
   - ハーフブロック文字の文字自体の色と背景色を別々に制御することで、1つの半角スペースを正方形ピクセル2つとして扱っています。
- 爆弾は連鎖します。
- 処理は全て時間ベースです（操作回数ベースではない）。
- websocket通信を用いてローカル・オンラインそれぞれのモードで2人対戦できます。
   - 一定時間相手が現れなかった際にはボットが自動的に現れます。
- 4種類のキャラクターから自機を選択するシステムがあり、キャラごとにスキルが設定されています。
- 以下の3種類のアイテムが実装されています。これらは壁を壊すことで入手できます。
  - 自機の移動が速くなる
  - 爆弾の設置数上限が増える
  - 爆弾の火が広く届くようになる（スキルによる爆弾には影響しない）
- Lineの公式アカウントを登録することで、遠くの誰かがマッチングをしている時に通知を受け取ることができます。[line公式アカウント](https://lin.ee/mb53vUP)

## ステージ仕様

### ステージ選択について

- ステージ選択では4種類のステージが選択できます。
  - グラフィックの見た目が違うものが2種類、仕様が異なるものが2種類で2×2となっています。
    - 見た目：mine（AmatuItadaki担当　鉱山風）、mansion（utyujinn担当　館風）
    - 仕様：前半は壁・アイテム少なめ、後半はいずれも多め

<table>
  <tr>
    <td align="center" width="50%">
      <img src="https://github.com/user-attachments/assets/be3c07e8-d0ed-4225-a533-98b9cd653dae" width="380" alt="Mine Stage"><br>
      <b>Mine（鉱山風）</b>
    </td>
    <td align="center" width="50%">
      <img src="https://github.com/user-attachments/assets/0a42bb00-50d4-4fe3-bd62-2552165235e0" width="380" alt="Mansion Stage"><br>
      <b>Mansion（館風）</b>
    </td>
  </tr>
</table>

- ステージを構成するブロック
  - 空白（blank）：特に何もなく、プレイヤーが通行したり爆弾を置いたりできます。
  - 硬い壁（wall）：プレイヤーが通行できず、破壊もできない壁です。
  - 弱い壁（stone）：そのままでは通行できませんが、爆弾の火が当たることで壊れます。壊れた際には一定確率でアイテムがドロップします。
- ステージ自体はランダム生成です。「アイテム多め」のステージはランダム生成の確率を高く設定したバージョンになります。
- ステージは2人のプレイヤーが選択したもののどちらかがランダムに選ばれます。

## キャラクター仕様

- 4人のキャラクターにそれぞれスキルがあり、ゲーム中に2回だけ発動できます。（QあるいはEnterキーで発動・アクション）
<table>
  <tr>
    <td width="200" align="center">
      <img src="https://github.com/user-attachments/assets/b131ecc3-aeee-445f-961c-2e406f6948f2" width="160" alt="Standing Image"><br><br>
      <img src="https://github.com/user-attachments/assets/979c8feb-7f4f-47ff-96d7-4bb4689d8ebe" width="60" alt="Icon">
    </td>
    <td valign="top">
      <h3>Irie <small>（ディジタル回路担当　入江教授のアバターの姿）</small></h3>
      <h4>～Binary World～</h4>
      <p>即時に爆発する爆弾を設置できる。スキル使用期間中は何度でも設置できる。</p>
      <ul>
        <li>スキルによって設置した爆弾で自爆することはない。安心して置ける。</li>
        <li>スキルで使える爆弾は通常のものより火の範囲が狭く、1マスサイズ。</li>
      </ul>
      <p>小ネタ：本人に許可を取りました。（とはいえあくまで二次創作、公式ではない）</p>
    </td>
  </tr>

  <tr>
    <td width="200" align="center">
      <img src="https://github.com/user-attachments/assets/9c4a98bb-995d-4843-ad0e-59730fec648a" width="160" alt="Kori Standing"><br><br>
      <img src="https://github.com/user-attachments/assets/0a54448e-e617-4ae2-a4e8-13cad731b64b" width="60" alt="Kori Icon">
    </td>
    <td valign="top">
      <h3>Kori <small>(Nyakori's rabbit doll)</small></h3>
      <h4>～Let me alone～</h4>
      <p>猫に変身し、壁（2種類いずれも）の上を移動することができる。</p>
      <ul>
        <li>スキル使用後に壁の上にいるとハマってしまって詰むので注意。</li>
      </ul>
    </td>
  </tr>

  <tr>
    <td width="200" align="center">
      <img src="https://github.com/user-attachments/assets/07409af6-f533-44d5-9bbe-3bcddb0dd22f" width="160" alt="Misaki Standing"><br><br>
      <img src="https://github.com/user-attachments/assets/ae516e14-6f16-4b8b-8ed1-b56bdc4be910" width="60" alt="Misaki Icon">
    </td>
    <td valign="top">
      <h3>Misaki <small>(製作者のオリキャラ)</small></h3>
      <h4>～StarMine～</h4>
      <p>爆弾の流星群を落とす。スキル発動と同時に落ちるので、「スキル使用中」というような期間の概念はない。</p>
      <ul>
        <li>こちらも落とされる爆弾の火の範囲は1マスです。</li>
      </ul>
    </td>
  </tr>

  <tr>
    <td width="200" align="center">
      <img src="https://github.com/user-attachments/assets/d1018344-f85e-471a-99de-8770fec55c38" width="160" alt="Toru Standing"><br><br>
      <img src="https://github.com/user-attachments/assets/a18d2df4-e7fa-447f-941e-fd3385980f4c" width="60" alt="Toru Icon">
    </td>
    <td valign="top">
      <h3>Toru <small>(Nyakori's rabbit doll)</small></h3>
      <h4>～I love fish～</h4>
      <p>魚を持って直接敵を殴ることができる。</p>
      <ul>
        <li>殴るアクション時にはグラフィックも動きます。</li>
        <li>自分が向いている方向の近くに敵がいるときのみ攻撃が当たります。</li>
      </ul>
    </td>
  </tr>
</table>

## コンパイル、実行方法

### **debian系**

```cmd
sudo apt install libwebsockets-dev
gcc bomber.c graphics.c graphics_game.c title.c help.c graphics_help.c graphics_title.c graphics_menu.c graphics_result.c client.c texture.c gamedata.c config.c -o bomber -lwebsockets
gcc server.c title.c menu.c game.c result.c bot.c config.c -o server -lwebsockets -lpthread
cp .env.example .env
./bomber
```

localモード時にも別ターミナルでもう一度`./bomber`を実行し、localモードを選択すると、Player2として接続されます。

### **redhat系** (サーバーでこちらを実行しています。)

```cmd
sudo dnf config-manager --set-enabled crb
sudo dnf makecache
sudo dnf install libwebsockets libwebsockets-devel -y
gcc bomber.c graphics.c graphics_game.c title.c help.c graphics_help.c graphics_title.c graphics_menu.c graphics_result.c client.c texture.c gamedata.c config.c -o bomber -lwebsockets
gcc server.c title.c menu.c game.c result.c bot.c config.c -o server -lwebsockets -lpthread
./server
```

### **nixos**

```cmd
nix-shell -p libwebsockets openssl.dev
gcc bomber.c graphics.c graphics_game.c title.c help.c graphics_help.c graphics_title.c graphics_menu.c graphics_result.c client.c texture.c gamedata.c config.c -o bomber -lwebsockets
gcc server.c title.c menu.c game.c result.c bot.c config.c -o server -lwebsockets -lpthread
cp .env.example .env
./bomber
```

### 注意点

ローカルモードのみ利用でもサーバーのコンパイルが必要です。（ローカルモードでは内部的にローカル用のサーバーを起動しているため）

## 設定

```cmd
cp .env.example .env
```

以上を実行した後に、.envに設定を書き込むことでサーバーを指定できます。bomber.utyujin.comで2025-12-3現在起動されています。

## 推しポイント

作ったプログラム全体を推したい……ところですが、いくつか抜粋して紹介します。

（AmatuItadaki）graphics.cにある画面を描画する関数ではいろいろな工夫が仕込まれています。
- 安直に全てのピクセルをエスケープシーケンス付きのprintfで常時出力すると、ターミナルが表示できる速度を超えてしまって描画が崩れる不具合が発生した。
   - そこで映像バッファーを2系統用意し、これらをポインタで切り替えて描画関数に渡す一方、実際の画面出力は前回のバッファーと変更された差分だけsprintf()とwrite()を使って少ない負荷で出力するようにした。
      - 映像バッファーはmemcpy等を使って管理することも可能と思われるが、ポインタを使うことでより軽量な動作が可能になっている（と思われる）。
- 基本的には描画にブロック文字を使って疑似的なピクセルとして扱っているが、ヘルプ画面やメニュー画面では通常の文字も併用する必要があった。これを併存させるために映像バッファーの形式を工夫した。
   - 1つのブロック文字は2つのピクセルを表す。ピクセルは24bit色を扱うため、int型の下位24bitを割り当てていて基本的に上位8bitは空白である。
   - 偶数段目のピクセル（screenBuff[i * 2][j]）の31bit目を「このピクセルは文字である」というフラグ、30bit目を「このピクセルは黒文字 + カスタム背景色を適用」というフラグにしている。
      - 31bit目のみ「1」の場合、int型の下位8bitをchar型として扱い、そこに格納されている文字の情報を出力する。（背景色は自動的に真っ黒になる）
      - 30bit目と31bit目が両方「1」の場合、奇数段目のピクセル（screenBuff[i * 2 + 1][j]）の24bit色を背景色とする、黒文字を出力する。これはゲーム中でプレイヤーが死亡した場合の表現（漫画的表現として目が×印になる）に使われている。

```c
void printScreen(){
    static int firstCall = 1;//初回呼び出しフラグ
    char screenTextBuff[BLOCK_X * FIELD_X * BLOCK_Y * FIELD_Y * 40];
    int numText = 0;
    numText += sprintf(screenTextBuff + numText, "\033[H");//カーソルを左上に移動
    for(int i = 0; i < (BLOCK_Y * FIELD_Y + FRAMEWIDTH * 2) / 2; i ++){
        for(int j = 0; j < BLOCK_X * FIELD_X + FRAMEWIDTH * 2; j ++){
            if(firstCall || screenBuff[i * 2][j] != screenBuffPrev[i * 2][j] || screenBuff[i * 2 + 1][j] != screenBuffPrev[i * 2 + 1][j]){//初回または前回と異なるピクセルを検出
                if(screenBuff[i * 2][j] & 0x80000000){//文字フラグが立っていたら文字として扱う
                    if(screenBuff[i * 2][j] & 0x40000000){//その一個下のビットがあれば文字は黒色にする（X_X顔用）
                        numText += sprintf(screenTextBuff + numText, "\033[%d;%dH\033[38;2;0;0;0m\033[48;2;%d;%d;%dm%c",
                            i + 1,
                            j + 1,
                            ((screenBuff[i*2+1][j] & 0xFF0000) >> 16),
                            ((screenBuff[i*2+1][j] & 0x00FF00) >> 8),
                            (screenBuff[i*2+1][j] & 0x0000FF),
                            (char)(screenBuff[i * 2][j] & 0x000000FF));
                    }else{
                        numText += sprintf(screenTextBuff + numText, "\033[%d;%dH\033[38;2;255;255;255m\033[48;2;0;0;0m%c", i + 1, j + 1, (char)(screenBuff[i * 2][j] & 0x000000FF));
                    }
                }else{//そうでなければ色塗りする
                    numText += sprintf(screenTextBuff + numText,
                    "\033[%d;%dH\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀",
                    i + 1,
                    j + 1,
                    ((screenBuff[i*2][j] & 0xFF0000) >> 16),
                    ((screenBuff[i*2][j] & 0x00FF00) >> 8),
                    (screenBuff[i*2][j] & 0x0000FF),
                    ((screenBuff[i*2+1][j] & 0xFF0000) >> 16),
                    ((screenBuff[i*2+1][j] & 0x00FF00) >> 8),
                    (screenBuff[i*2+1][j] & 0x0000FF));
                }
            }
        }
    }
    numText += sprintf(screenTextBuff + numText, "\033[%d;1H", BLOCK_Y * FIELD_Y / 2 + 2);
    write(STDOUT_FILENO, screenTextBuff, numText);

    firstCall = 0;//初回フラグをオフ
    int (*screenBuffForSwap)[BLOCK_X * FIELD_X + FRAMEWIDTH * 2] = screenBuffPrev;//screenBuffのポインタを入れ替える
    screenBuffPrev = screenBuff;
    screenBuff = screenBuffForSwap;
}
```

（AmatuItadaki）ビット演算を駆使することによって、変数の数を減らしつつ関連する関数で流用しやすいデータ構造を目指しました。
例えばプレイヤーのステータス（char型　8bit）では以下の3つの情報が入っています。
- ビット0~1 : アイテムA所持フラグ （0 : ノーマル、1 : 1段階加速、2 : 2段階加速）
- ビット2~4 : 火の長さ（デフォルトで2、アイテム取得で3など増える）
- ビット5~6 : スキル使用可能回数

また、プレイヤーのスキルが有効な時間を表すskillTimerは、単に時間を管理するだけでなく0以外かを判定することで「スキルを使っているモードに入っているか」の判断に使ったり、グラフィックを切り替えたりするために流用されています。

```c
if((PlayerPointer->status & 0b01100000) != 0){
   PlayerPointer->status -= 0b00100000;
   if(PlayerPointer->name != MISAKI){
      PlayerPointer->skillTimer = 100;//タイマーとして使う部分
   }else{
      skill_MISAKI(world);
   }
}

if(player_pointer(world, player)->skillTimer == 0){//スキル発動中かの判定（Qキーを「スキル発動」に使うか「スキルのアクション」に使うかの判定）
   activate_skill(world, player);
}else{
   if(player_pointer(world, player)->name == IRIE){
      skill_IRIE(world, player);
   }else if(player_pointer(world, player)->name == TORU){
      skill_TORU(world, player);
   }
}

if(world->player1.skillTimer != 0){//スキル発動中かの判定（描画用）
   if(!(world->player1.skillTimer < 25 && world->player1.skillTimer % 2 == 0)){//さらにスキル使用時間切れが近づくと点滅する制御にも
      player1Diff += 4;
   }
}
```

（utyujinn）client.cにて、接続時にデータやり取りを行うことで、通信相手の勝利数を見れるようにしました。

```c
case LWS_CALLBACK_CLIENT_ESTABLISHED:
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
```

（utyujinn）server.cにて、クライアント接続時にLineへと通知を送るようにしました。これにより、ゲームが過疎っていてもマッチングしやすくなります。

```c
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
```

（utyujinn）ファイル分割をしました。グラフィックとロジック部分を完全に分離することで、サーバーでゲームを処理して、クライアント側では変化したゲーム状態を受け取るのみでゲームを進行することができます。

```cmd
$ tree 
.
├── bomber.c
├── bot.c
├── bot.h
├── client.c
├── client.h
├── config.c
├── config.h
├── game.c
├── gamedata.c
├── gamedata.h
├── gamedata.txt
├── game.h
├── graphics.c
├── graphics_game.c
├── graphics_game.h
├── graphics.h
├── graphics_help.c
├── graphics_help.h
├── graphics_menu.c
├── graphics_menu.h
├── graphics_result.c
├── graphics_result.h
├── graphics_title.c
├── graphics_title.h
├── help.c
├── help.h
├── menu.c
├── menu.h
├── README.md
├── result.c
├── result.h
├── server.c
├── texture.c
├── texture.h
├── title.c
├── title.h
└── types.h
```

## 参考サイト等

基本的には参考にして書いたコードの近辺にコメントアウトで記されています。
以下にも写して列挙します。

- システムプログラミング入門 渡辺知恵美著 サイエンス社
- [libwebsockets公式ドキュメント](https://libwebsockets.org)
- [simple-libwebsockets-example](https://github.com/iamscottmoyers/simple-libwebsockets-example/tree/master)
- [RGBとHSV・HSBの相互変換ツールと変換計算式](https://www.peko-step.com/tool/hsvrgb.html#ppick2)
- [コンソールグラフィック](https://www.mm2d.net/main/prog/c/console-03.html)
- [C言語エスケープシーケンスチートシート](https://qiita.com/sudo00/items/2b2eec07d3099b5ad664)
- [write()ファイルデスクリプタへ書き込む](https://cgengo.sakura.ne.jp/write.html)
- [ユーザーのライブラリを作る。makefile のテンプレート、getch(),kbhit()を例にとって](https://qiita.com/fygar256/items/7ac58d13cf71928c52dd)
- [インクルードガード](https://teratail.com/questions/353218)
- [claude code](https://claude.com/product/claude-code) Websocket通信周りで使用しました。ドット絵を作成するユーティリティ(bitmapディレクトリ)の作成にも用いました。
