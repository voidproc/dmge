#pragma once

namespace dmge
{
	struct GamepadButtonAssign
	{
		int buttonA = 0;
		int buttonB = 1;
		int buttonSelect = 2;
		int buttonStart = 3;
	};

	struct AppConfig
	{
		// 読み込むカートリッジのパス
		String cartridgePath{};

		// カートリッジを開くダイアログのデフォルトディレクトリ
		String openCartridgeDirectory{};

		// ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> breakpoints{};

		// メモリ書き込み時ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> memoryWriteBreakpoints{};

		// トレースダンプを開始するアドレス
		Array<uint16> traceDumpStartAddress{};

		// メモリダンプ箇所
		// 指定されたアドレスから16バイト分をダンプする
		Array<uint16> dumpAddress{};

		// ブレークポイントを使用する
		bool enableBreakpoint = false;

		// FPSを表示する
		bool showFPS = true;

		// 表示倍率
		int scale = 3;

		// CGB モードでのカラー補正（ガンマ値）
		double cgbColorGamma = 1.0;

		// コンソールを表示する
		bool showConsole = true;

		// ログ出力先
		String logFilePath{};

		// デバッグモニタを表示する
		bool showDebugMonitor = true;

		// LD B,B 実行時にブレークする
		bool breakOnLDBB = false;

		// Bootstrap ROMのパス
		String bootROMPath{};

		// ゲームパッドのボタン割り当て
		GamepadButtonAssign gamepadButtonAssign;

		bool testMode = false;

		// config.ini のパース結果を返す
		static AppConfig LoadConfig();

		void print();
	};
}
