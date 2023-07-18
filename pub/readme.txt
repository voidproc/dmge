# dmge - DMG Emulator

https://github.com/voidproc/dmge

C++ / OpenSiv3D でゲームボーイのエミュレータを作っています。


## 概要

C++ / OpenSiv3D でゲームボーイのエミュレータを作っています。ゲームボーイの構造や動作原理、そしてエミュレータの開発に関する理解を深めるための、主に学習目的のプロジェクトです。

正確性や効率性、ユーザーインターフェースに関することの優先度は低く、特筆すべき機能もありません。そのため、何かのカートリッジイメージを動作させることが目的であれば、[BGB](https://bgb.bircd.org/) や [SameBoy](https://sameboy.github.io/) のような優れたエミュレータの使用をおすすめします。


## 開発環境

- Windows 11
- Visual Studio 2022
- [OpenSiv3D](https://github.com/Siv3D/OpenSiv3D) v0.6.10


## 使用方法

`dmge.exe` を起動してください。

以下のキーボードショートカットが使用可能です。

- ボタン操作
  - 4方向ボタン : 矢印キー
  - A ボタン : X
  - B ボタン : Z
  - Start ボタン : Enter
  - Select ボタン : BackSpace
- ファイル
  - カートリッジを開くダイアログ表示 : Ctrl + O
  - リセット : Ctrl + R
- レンダリング
  - パレット切替（DMG モード） : L
- サウンド
  - チャンネル 1 のミュートを切替 : 1
  - チャンネル 2 のミュートを切替 : 2
  - チャンネル 3 のミュートを切替 : 3
  - チャンネル 4 のミュートを切替 : 4
  - サウンド有効／無効切替 : 5
- デバッグ
  - デバッグモニタ表示切替 : F10
  - デバッグモニタのメモリダンプ対象アドレス設定 : M
  - ステップ実行を開始 : P
  - ステップ実行 : F7
  - ステップ実行を終了 : F5

また、`config.ini` を編集して以下の設定を行うことができます（`config.example.ini` を参考にしてください）。

- `CartridgePath` : 読み込むカートリッジファイルのパス
- `OpenCartridgeDirectory` : カートリッジを開くダイアログのデフォルトディレクトリ
- `Breakpoint` : ブレークポイント（コンマ区切りで複数指定可能）
- `MemoryWriteBreakpoint` : メモリ書き込み時ブレークポイント（コンマ区切りで複数指定可能）
- `BreakOnLDBB` : LD B,B 実行時にブレークするかどうか
- `EnableBreakpoint` : 上記ブレークポイントの有効／無効の切替
- `TraceDumpStartAddress` : トレースダンプを開始するアドレス（コンマ区切りで複数指定可能）
- `ShowFPS` : 画面上部にFPSを表示するかどうか
- `Scale` : 画面の表示倍率
- `CGBColorGamma` : CGB モードでのカラー補正（ガンマ値）
- `ShowConsole` : コンソールを表示するかどうか
- `LogFilePath` : トレースダンプなどの出力先のパス
- `ShowDebugMonitor` : デバッグモニタを表示するかどうか
- `DumpAddress` : ブレーク時のメモリダンプ先アドレス（コンマ区切りで複数指定可能）
- `BootROM` : Bootstrap ROM のパス
- `GamepadButton****` : ゲームパッドのボタン割り当て
