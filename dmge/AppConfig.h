#pragma once

#include "InputMappingArray.h"

namespace dmge
{
	struct AppConfig
	{
		static inline constexpr StringView ConfigFilePath = U"config.ini"_sv;
		static inline constexpr int ScalingMax = 8;

		// config.ini のパース結果を返す
		static AppConfig LoadConfig();

		void save();

		void print();


		// --------------------------------
		// Cartridge
		// --------------------------------

		// 読み込むカートリッジのパス
		String cartridgePath{};

		// カートリッジを開くダイアログのデフォルトディレクトリ
		String openCartridgeDirectory{};

		// カートリッジが対応していれば CGB モードで実行する
		bool detectCGB = true;

		// カートリッジが対応していれば SGB モードで実行する
		bool detectSGB = true;

		// Bootstrap ROMのパス
		String bootROMPath{};


		// --------------------------------
		// Display
		// --------------------------------

		// 表示倍率
		int scale = 3;

		// FPSを表示する
		bool showFPS = false;

		// (DMG/SGB) パレット番号
		int palettePreset = 0;

		// (DMG/SGB) カスタムパレットカラー
		std::array<ColorF, 4> paletteColors{
			ColorF{ U"#e8e8e8" }, ColorF{ U"#a0a0a0" }, ColorF{ U"#585858" }, ColorF{ U"#101010" }
		};

		// CGB モードでのカラー補正（ガンマ値）
		double cgbColorGamma = 1.0;


		// --------------------------------
		// DebugMonitor
		// --------------------------------

		// デバッグモニタを表示する
		bool showDebugMonitor = false;


		// --------------------------------
		// Input
		// --------------------------------

		// キーボード割り当て
		InputMappingArray keyMapping = { KeyRight, KeyLeft, KeyUp, KeyDown, KeyX, KeyZ, KeyBackspace, KeyEnter };

		// ゲームパッドのボタン割り当て
		InputMappingArray gamepadMapping;


		// --------------------------------
		// Audio
		// --------------------------------

		// オーディオにローパスフィルタを適用する
		bool enableAudioLPF = false;

		// ローパスフィルタの定数
		double audioLPFConstant = 0.8;


		// --------------------------------
		// Debug
		// --------------------------------

		// コンソールを表示する
		bool showConsole = true;

		// ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> breakpoints{};

		// メモリ書き込み時ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> memoryWriteBreakpoints{};

		// LD B,B 実行時にブレークする
		bool breakOnLDBB = false;

		// ブレークポイントを使用する
		bool enableBreakpoint = false;

		// メモリダンプ箇所
		// 指定されたアドレスから16バイト分をダンプする
		Array<uint16> dumpAddress{};

		// トレースダンプを開始するアドレス
		Array<uint16> traceDumpStartAddress{};

		// ログ出力先
		String logFilePath{};

		// テストROM実行モード
		bool testMode = false;
	};
}
