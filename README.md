# pochi-pochi

## 概要

`pochi-pochi` は、Webブラウザと物理的な16x16 LEDマトリックスの両方で動作するように設計されたシンプルなピクセルゲームです。ゲームの操作はボタン1つだけで行います。

このリポジトリには、コアとなるC++のゲームロジック、WebAssemblyにコンパイルするためのビルドスクリプト、およびArduino互換のハードウェアで実行するためのスケッチが含まれています。

## 特徴

- **クロスプラットフォーム**: WebブラウザとArduinoベースのハードウェアの両方で同じゲームロジックが動作します。
- **シンプルな操作**: ジャンプやアクションはすべて1つのボタンで行います。
- **16x16ピクセルアート**: 懐かしい雰囲気のシンプルなピクセルグラフィックスタイルです。
- **複数のゲームモード**: ジャンプゲームやチェイスゲームなど、異なるゲームロジックが含まれています。（`game_jump.cpp`, `game_chase.cpp`）

## 実行方法

### Web (ブラウザ) 版

Web版は、C++のコードをWebAssemblyにコンパイルして実行します。

1.  **前提条件**: [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) がインストールされている必要があります。
2.  **ビルド**: 以下のコマンドを実行して、ゲームをコンパイルします。
    ```bash
    sh build.sh
    ```
    これにより、`public` ディレクトリに `game.js` と `game.wasm` が生成されます。
3.  **実行**: `public` ディレクトリをローカルサーバーでホストし、`index.html`にアクセスします。
    ```bash
    # 例: Pythonのhttp.serverを使用する場合
    cd public
    python -m http.server
    ```
    ブラウザで `http://localhost:8000` を開きます。

### ハードウェア (Arduino) 版

ハードウェア版は、Arduino互換ボードとLEDマトリックスで動作します。

1.  **必要な機材**:
    - Arduino互換マイクロコントローラ (例: Arduino Uno, ESP32)
    - 16x16 NeoPixel Matrix
    - プッシュボタン x 1
    - ブレッドボードと配線材

2.  **配線**:
    - NeoPixel Matrix のデータ入力ピンを Arduino の **ピン6** に接続します。
    - プッシュボタンを Arduino の **ピン2** と **GND** に接続します。（スケッチは内部プルアップ抵抗を使用します）

3.  **セットアップ**:
    - [Arduino IDE](https://www.arduino.cc/en/software) をインストールします。
    - ライブラリマネージャから以下のライブラリをインストールします:
        - `Adafruit GFX Library`
        - `Adafruit NeoMatrix`
        - `Adafruit NeoPixel`
    - Arduino IDEで `simple-dot/simple-dot.ino` を開きます。

4.  **実行**:
    - Arduino IDEで正しいボードとポートを選択します。
    - 「アップロード」ボタンをクリックして、スケッチをマイクロコントローラに書き込みます。

## プロジェクト構造

```
.
├── build.sh             # WebAssembly版をビルドするシェルスクリプト
├── public/              # Web版のファイル（HTML, JS, WASM）
│   ├── game.js
│   ├── game.wasm
│   ├── index.html
│   └── main.js
└── src/                 # ゲームのコアロジックとArduinoスケッチ
    ├── game_chase.cpp   # チェイスゲームのロジック
    ├── game_jump.cpp    # ジャンプゲームのロジック
    ├── game_logic.cpp   # 共通のゲームロジック
    ├── *.h              # 各ソースコードのヘッダーファイル
    └── simple-dot.ino   # Arduino用スケッチ
```
