# dmge - DMG Emulator

https://github.com/voidproc/dmge

C++ / OpenSiv3D でゲームボーイのエミュレータを作っています。


## 概要

C++ / OpenSiv3D でゲームボーイのエミュレータを作っています。ゲームボーイの構造や動作原理、そしてエミュレータの開発に関する理解を深めるための、主に学習目的のプロジェクトです。

正確性や効率性、ユーザーインターフェースに関することの優先度は低く、特筆すべき機能もありません。そのため、何かのカートリッジイメージを動作させることが目的であれば、[BGB](https://bgb.bircd.org/) や [SameBoy](https://sameboy.github.io/) のような優れたエミュレータの使用をおすすめします。


## 開発環境

- Windows 11
- Visual Studio 2022
- [OpenSiv3D](https://github.com/Siv3D/OpenSiv3D) v0.6.11


## 使用方法

### アプリケーションの起動

`dmge.exe` を起動してください。

### メニュー

右クリック、Escape キー、または左クリック長押しでメニューが表示されます。次に示す各種操作や設定変更を行うことができます。

- リセット
- カートリッジを開く（ダイアログ表示）
- ポーズ（ステップ実行開始）
- 画面サイズ変更（x1 ～ x8）
- パレットカラーのプリセットを切替（DMG/SGB モード）
- コントローラ設定（キーボード／ゲームパッドのボタン割り当て）
- オーディオ有効／無効切替
- オーディオチャンネル 1～4 のミュート切替
- オーディオの LPF 有効／無効切替
- デバッグモニタ表示切替
- FPS 表示切替

メニューはキーボードまたはマウスで操作できます。

- 選択: 矢印キー（↑、↓）、マウスホイール
- 決定／値の変更: Enter、左クリック
- 値の変更（増減）: 矢印キー（←、→）
- 戻る: Escape、右クリック

### キーボードショートカット

以下のキーボードショートカットが使用できます。

- ボタン操作（カスタマイズ可能）
  - 4方向ボタン : 矢印キー
  - A ボタン : X
  - B ボタン : Z
  - Start ボタン : Enter
  - Select ボタン : BackSpace
- ファイル
  - カートリッジを開くダイアログ表示 : Ctrl + O
  - リセット : Ctrl + R
- レンダリング
  - パレットカラーのプリセットを切替（DMG/SGB モード） : Ctrl + L
- オーディオ
  - チャンネル 1 のミュートを切替 : 1
  - チャンネル 2 のミュートを切替 : 2
  - チャンネル 3 のミュートを切替 : 3
  - チャンネル 4 のミュートを切替 : 4
  - オーディオ有効／無効切替 : 5
  - オーディオの LPF 有効／無効切替 : 6
- デバッグ
  - デバッグモニタ表示切替 : F10
  - デバッグモニタのメモリダンプ対象アドレス設定 : Ctrl + M
  - ステップ実行を開始／終了 : Ctrl + P
  - ステップ実行 : F7
  - ステップ実行を終了 : F5
  - コンソールへ常に実行結果をダンプする : Ctrl + D

### 設定（`config.ini`）

各種設定は起動時に `config.ini` から読み込まれ、アプリケーション終了時に**上書き保存されます** 。

設定変更は上記のメニューから行うことができます。または `config.ini` ファイルを直接編集することも可能です。

設定項目は次のとおりです（`config.example.ini` を参考にしてください）。

- カートリッジ関連
  - `CartridgePath` : 読み込むカートリッジファイルのパス
  - `OpenCartridgeDirectory` : カートリッジを開くダイアログのデフォルトディレクトリ
  - `DetectCGB` : カートリッジが対応していれば CGB モードで実行する（1=有効、0=無効）
  - `DetectSGB` : カートリッジが対応していれば SGB モードで実行する（1=有効、0=無効）
  - `BootROM` : Bootstrap ROM のパス
- 画面表示関連
  - `Scale` : 画面の表示倍率
  - `ShowFPS` : 画面上部にFPSを表示する（1=有効、0=無効）
  - `PalettePreset` : パレットカラーのプリセット番号（0～8、0番はカスタムカラー） (DMG/SGB)
  - `PaletteColor0` ～ `PaletteColor3` : カスタムカラー (DMG/SGB)
  - `CGBColorGamma` : CGB モードでのカラー補正（ガンマ値）
- デバッグモニタ関連
  - `ShowDebugMonitor` : デバッグモニタを表示する（1=有効、0=無効）
- 入力関連
  - `Keyboard****` : キーボードの割り当て
    - `KeyboardRight`, `KeyboardLeft`, `KeyboardUp`, `KeyboardDown`, `KeyboardA`, `KeyboardB`, `KeyboardSelect`, `KeyboardStart`
  - `GamepadButton****` : ゲームパッドのボタン割り当て
    - `GamepadButtonA`, `GamepadButtonB`, `GamepadButtonSelect`, `GamepadButtonStart`
- オーディオ関連
  - `EnableAudioLPF` : オーディオにローパスフィルタを適用する（1=有効、0=無効）
  - `AudioLPFConstant` : ローパスフィルタの定数（0～1）
- デバッグ関連
  - `ShowConsole` : コンソールを表示する（1=有効、0=無効）
  - `Breakpoint` : ブレークポイント（コンマ区切りで複数指定可能）
  - `MemoryWriteBreakpoint` : メモリ書き込み時ブレークポイント（コンマ区切りで複数指定可能）
  - `BreakOnLDBB` : LD B,B 実行時にブレークする（1=有効、0=無効）
  - `EnableBreakpoint` : 上記ブレークポイントの有効／無効の切替（1=有効、0=無効）
  - `DumpAddress` : ブレーク時のメモリダンプ先アドレス（コンマ区切りで複数指定可能）
  - `TraceDumpStartAddress` : トレースダンプを開始するアドレス（コンマ区切りで複数指定可能）
  - `LogFilePath` : トレースダンプなどの出力先のパス
  - `TestMode` : テスト ROM 実行モード（1=有効、0=無効）
  