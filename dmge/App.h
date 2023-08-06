#pragma once

#include "Test.h"
#include "GUI/Menu.h"

namespace dmge
{
	// アプリケーションの状態
	enum class DmgeAppMode
	{
		// エミュレーションを実行している
		Default,

		// トレースモード
		// CPU 命令を 1 つ実行するごとにポーズ（ブレーク）する
		Trace,

		// リセットまたは他のカートリッジの読み込みを行う
		Reset,
	};

	struct AppConfig;
	class Memory;
	class Interrupt;
	class LCD;
	class PPU;
	class APU;
	class Timer;
	class CPU;
	class Joypad;
	class Serial;
	class DebugMonitor;
	class InputMapping;

	// アプリケーション
	class DmgeApp
	{
	public:
		DmgeApp(AppConfig& config);

		~DmgeApp();

		void run();

		void setCartridgePath(const String& cartridgePath);

		String currentCartridgePath() const;

		DmgeAppMode mode() const;

		MooneyeTestResult mooneyeTestResult() const;

	private:

		void mainLoop_();

		void traceLoop_();

		void menuLoop_();

		void commonInput_();

		// メモリ書き込み時フック
		void onMemoryWrite_(uint16 addr, uint8 value);

		// 画面表示用パレットを設定する
		void setPPUPalette_(int paletteIndex);

		// ブレークポイントが有効かつブレークポイントに達したか
		bool reachedBreakpoint_() const;

		bool reachedTraceDumpAddress_() const;

		void tickUnits_(int cycles);

		bool checkShouldDraw_();

		void updateDebugMonitor_();

		void initMenu_();

		void reset_();

		void pause_();

		void resume_();

		void openCartridge_();

		void toggleAudioChannelMute_(int channel);

		void toggleAudio_();

		void toggleAudioLPF_();

		void toggleDebugMonitor_(bool applyWindowSize);

		void toggleShowFPS_();

		void changePalettePreset_(int changeIndex);

		// キー／ボタンのマッピングが変更されたので Joypad と config に変更を適用する
		void applyInputMapping_();

		AppConfig& config_;

		std::unique_ptr<Memory> mem_;
		std::unique_ptr<Interrupt> interrupt_;
		std::unique_ptr<LCD> lcd_;
		std::unique_ptr<PPU> ppu_;
		std::unique_ptr<Timer> timer_;
		std::unique_ptr<APU> apu_;
		std::unique_ptr<CPU> cpu_;
		std::unique_ptr<Joypad> joypad_;
		std::unique_ptr<Serial> serial_;
		std::unique_ptr<DebugMonitor> debugMonitor_;
		std::unique_ptr<InputMapping> keyMap_;
		std::unique_ptr<InputMapping> gamepadMap_;

		GUI::Menu rootMenu_;
		GUI::Menu inputMenu_;
		GUI::MenuOverlay menuOverlay_{ config_ };

		// アプリケーションの状態
		DmgeAppMode mode_ = DmgeAppMode::Default;

		// アプリケーションを終了する
		bool quitApp_ = false;

		// APUを使用する
		bool enableAPU_ = true;

		// (APU)サンプリングレート
		int sampleRate_ = 44100;

		// 現在の画面表示用パレット番号
		//int currentPalette_ = 0;

		// トレースダンプする
		bool enableTraceDump_ = false;

		// メモリダンプするアドレス設定用
		uint16 dumpAddress_ = 0;

		// ロードしているカートリッジのパス
		Optional<String> currentCartridgePath_{};

		// オーディオストリームに書き込んだサンプル数の合計
		// 1フレーム分書き込んだら描画に移る
		int bufferedSamples_ = 0;

		// 前回の描画からの経過サイクル数
		// 一定のサイクル数を超過したら描画に移る
		int cyclesFromPreviousDraw_ = 0;

		// テスト ROM (Mooneye) の実行結果
		MooneyeTestResult mooneyeTestResult_ = MooneyeTestResult::Running;
	};
}
