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
		static inline constexpr int ScalingMax = 8;

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

		// オーディオにローパスフィルタを適用する
		bool enableAudioLPF = false;

		// ローパスフィルタの定数
		double audioLPFConstant = 0.8;

		// カートリッジが対応していれば CGB モードで実行する
		bool detectCGB = true;

		// カートリッジが対応していれば SGB モードで実行する
		bool detectSGB = true;

		// (DMG/SGB) パレット番号
		int palettePreset = 0;

		// (DMG/SGB) カスタムパレットカラー
		std::array<ColorF, 4> paletteColors{
			ColorF{ U"#e8e8e8" }, ColorF{ U"#a0a0a0" }, ColorF{ U"#585858" }, ColorF{ U"#101010" }
		};

		// config.ini のパース結果を返す
		static AppConfig LoadConfig();

		void print();
	};
}
