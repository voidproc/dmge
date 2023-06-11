# include <Siv3D.hpp> // OpenSiv3D v0.6.9

#include "Memory.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Address.h"
#include "Timing.h"
#include "AppConfig.h"
#include "DebugPrint.h"
#include "DebugMonitor.h"


void LoadAssets()
{
	const auto preloadText = U"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ=-~^|@`[{;+:*]},<.>/?_";

	FontAsset::Register(U"debug", 10, U"fonts/JF-Dot-MPlus10.ttf", FontStyle::Bitmap);
	FontAsset::Load(U"debug", preloadText);
}

void SetWindowSize(int scale, bool showDebugMonitor)
{
	const int width = 160 * scale + (showDebugMonitor ? 5 * 58 : 0);
	const int height = Max(144 * scale, showDebugMonitor ? 10 * 32 : 144 * scale);

	const auto SceneSize = Size{ width, height };
	Scene::Resize(SceneSize);
	Window::Resize(SceneSize);
}

void InitScene(int scale, bool showDebugMonitor)
{
	SetWindowSize(scale, showDebugMonitor);

	Scene::SetBackground(Palette::Whitesmoke);

	Scene::SetTextureFilter(TextureFilter::Nearest);

	Graphics::SetVSyncEnabled(true);

	System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	Profiler::EnableAssetCreationWarning(false);
}

void DrawStatusText(StringView text)
{
	const Size size{ 5 * text.length(), 10 };
	const Rect region{ Scene::Rect().tr().movedBy(-size.x, 0), size };
	region.stretched(1, 1).draw(Color{0, 128});
	FontAsset(U"debug")(text).draw(region.pos);
}


class DmgeApp
{
	enum class Mode
	{
		Default,
		Trace,
	};

public:
	DmgeApp()
	{
		if (config_.showConsole)
		{
			dmge::DebugPrint::EnableConsole();
		}

		showDebugMonitor_ = config_.showDebugMonitor;

		config_.print();

		mem_.init(&ppu_, &apu_, &timer_, &joypad_);

		// メモリ書き込み時フックを設定
		if (config_.enableBreakpoint && not config_.breakpointsMemWrite.empty())
		{
			mem_.setWriteHook([&](uint16 addr, uint8 value) {
				onMemoryWrite_(addr, value);
			});
		}

		// フォントなどのアセット読み込み
		LoadAssets();

		// Siv3Dのシーン・ウィンドウなどの初期化
		InitScene(config_.scale, showDebugMonitor_);

		// 画面表示用パレット初期化
		setPPUPalette_(0);


	}

	void run()
	{
		String cartridgePath = config_.cartridgePath;

		// 設定ファイルでカートリッジが指定されていない場合は
		// ファイルを開くダイアログで選択する
		// ファイルが存在しない or キャンセルされた場合は終了する
		if (not FileSystem::Exists(cartridgePath) || not FileSystem::IsFile(cartridgePath))
		{
			const auto cartDir = FileSystem::PathAppend(FileSystem::InitialDirectory(), U"cartridges");
			const auto openPath = Dialog::OpenFile({ FileFilter{ .name = U"GAMEBOY Cartridge", .patterns = {U"gb?"} } }, cartDir, U"ファイルを開く");

			if (not openPath)
			{
				dmge::DebugPrint::Writeln(U"Cartridge not found");
				return;
			}

			cartridgePath = *openPath;
		}

		// カートリッジ読み込み
		// 対応するMBCがカートリッジとSRAMをロードする
		// 対応するMBCがない場合は終了する
		if (not mem_.loadCartridge(cartridgePath))
		{
			return;
		}

		// [DEBUG]
		mem_.dumpCartridgeInfo();

		// CGBモードの適用
		ppu_.setCGBMode(mem_.isCGBMode());
		cpu_.setCGBMode(mem_.isCGBMode());
		cpu_.reset();

		mainLoop_();

		// アプリケーション終了時にSRAMを保存する
		mem_.saveSRAM();
	}

private:

	void mainLoop_()
	{
		// 前回の描画からの経過サイクル数
		uint64 cyclesFromPreviousDraw = 0;

		bool shouldDraw = false;

		bool alwaysDump = false;

		bool enableAPU = true;

		int bufferedSamples = 0;

		//dmge::FPSKeeper fpsKeeper{};


		while (not quitApp_)
		{
			// ブレークポイントが有効で、ブレークポイントに達していたら
			// トレースモードに切り替える

			if (reachedBreakpoint_())
			{
				mode_ = Mode::Trace;

				apu_.pause();

				dmge::DebugPrint::Writeln(U"Break: pc={:04X}"_fmt(cpu_.currentPC()));
			}


			// [DEBUG]
			if (alwaysDump)
			{
				cpu_.dump();
			}

			// トレースモードなら、専用のループへ

			if (mode_ == Mode::Trace)
			{
				for (const uint16 addr : config_.dumpAddress)
				{
					mem_.dump(addr, addr + 15);
				}

				cpu_.dump();

				traceLoop_();
			}

			// CPUコマンドを1回実行する

			cpu_.applyScheduledIME();
			cpu_.run();

			// タイマーを更新

			for (int i : step(cpu_.consumedCycles()))
			{
				timer_.update();
			}

			// PPU

			for (int i : step(cpu_.consumedCycles()))
			{
				ppu_.run();
			}

			// APU

			if (enableAPU)
			{
				for (int i : step(cpu_.consumedCycles()))
				{
					bufferedSamples += apu_.run();
				}
			}

			// 割り込み

			cpu_.interrupt();


			// 描画するか？

			if (enableAPU)
			{
				// (1) オーディオバッファに1フレーム分のサンプルを書き込んだら描画する

				if (bufferedSamples > 1.0 * sampleRate_ / 60.0)
				{
					shouldDraw = true;

					bufferedSamples -= sampleRate_ / 60.0;
				}
			}

			// (2) 描画されないまま一定のサイクル数が経過した場合に強制的に描画する

			cyclesFromPreviousDraw += cpu_.consumedCycles();

			if (cyclesFromPreviousDraw >= dmge::ClockFrequency / 59.50)
			{
				shouldDraw = true;
			}


			// キー入力と描画

			if (shouldDraw)
			{
				if (not System::Update())
				{
					quitApp_ = true;
					return;
				}

				commonInput_();

				// トレースモードに移行
				if (KeyP.down())
				{
					mode_ = Mode::Trace;

					apu_.pause();
				}

				// [DEBUG]常にダンプ
				if (KeyD.down())
				{
					alwaysDump = not alwaysDump;
				}

				// [DEBUG]
				if (Key1.down())
				{
					enableAPU = not enableAPU;
					if (not enableAPU)
					{
						apu_.pause();
					}
				}

				// ボタン入力の更新
				// ※デバッグモニタのテキストボックス入力中は更新しない
				if (not (showDebugMonitor_ && debugMonitor_.isVisibleTextbox()))
				{
					joypad_.update();
				}

				// PPUのレンダリング結果を画面表示
				ppu_.draw(Point{ 0, 0 }, config_.scale);

				// APU
				if (enableAPU && mode_ != Mode::Trace)
				{
					apu_.playIfBufferEnough(2000);
					apu_.pauseIfBufferNotEnough(512);
				}

				// デバッグ用モニタ表示
				if (showDebugMonitor_)
				{
					debugMonitor_.update();

					debugMonitor_.draw(Point{ 160 * config_.scale, 0 });
				}

				if (config_.showFPS)
				{
					DrawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
				}

				// Wait

				//if (not Graphics::IsVSyncEnabled())
				//{
				//	fpsKeeper.sleep(60.0 * 70224 / Max(cyclesFromPreviousDraw, 1ull));
				//}

				shouldDraw = false;
				cyclesFromPreviousDraw = 0;
			}
		}
	}

	void traceLoop_()
	{
		while (mode_ == Mode::Trace)
		{
			if (not System::Update())
			{
				quitApp_ = true;
				break;
			}

			commonInput_();

			// ステップ実行
			if (KeyF7.down())
			{
				break;
			}

			// トレースモード終了
			if (KeyF5.down())
			{
				mode_ = Mode::Default;
				break;
			}

			ppu_.drawCache(Point{ 0, 0 }, config_.scale);

			// デバッグ用モニタ表示
			if (showDebugMonitor_)
			{
				debugMonitor_.update();

				debugMonitor_.draw(Point{ 160 * config_.scale, 0 });
			}

			DrawStatusText(U"TRACE");
		}
	}

	void commonInput_()
	{
		// デバッグモニタ表示切り替え
		if (KeyF10.down())
		{
			showDebugMonitor_ = not showDebugMonitor_;
			SetWindowSize(config_.scale, showDebugMonitor_);
		}

		// パレット切り替え
		if (KeyL.down())
		{
			currentPalette_ = (currentPalette_ + 1) % paletteList_.size();
			setPPUPalette_(currentPalette_);
		}
	}

	// メモリ書き込み時フック
	void onMemoryWrite_(uint16 addr, uint8 value)
	{
		if (not config_.enableBreakpoint || mode_ == Mode::Trace)
		{
			return;
		}

		for (const auto& mem : config_.breakpointsMemWrite)
		{
			if (addr == mem)
			{
				mode_ = Mode::Trace;

				apu_.pause();

				dmge::DebugPrint::Writeln(U"Break(Memory Write): pc={:04X} mem={:04X} val={:02X}"_fmt(cpu_.currentPC(), addr, value));
				cpu_.dump();
				break;
			}
		}
	}

	// 画面表示用パレットを設定する
	void setPPUPalette_(int paletteIndex)
	{
		ppu_.setDisplayColorPalette(paletteList_[paletteIndex]);
		Scene::SetBackground(paletteList_[paletteIndex][0]);
	}

	// ブレークポイントが有効かつブレークポイントに達したか
	bool reachedBreakpoint_()
	{
		if (not config_.enableBreakpoint) return false;

		for (const auto bp : config_.breakpoints)
		{
			if (cpu_.currentPC() == bp)
			{
				return true;
			}
		}

		return false;
	}


	dmge::AppConfig config_ = dmge::AppConfig::LoadConfig();

	dmge::Memory mem_{};

	dmge::PPU ppu_{ &mem_ };

	// (APU)サンプリングレート
	int sampleRate_ = 44100;

	dmge::APU apu_{ &mem_, sampleRate_ };

	dmge::Timer timer_{ &mem_ };

	dmge::CPU cpu_{ &mem_ };

	dmge::Joypad joypad_{ &mem_ };

	dmge::DebugMonitor debugMonitor_{ &mem_, &cpu_, &apu_ };

	// モード（通常 or トレース）
	Mode mode_ = Mode::Default;

	// アプリケーションを終了する
	bool quitApp_ = false;

	// 画面表示用パレット
	const Array<std::array<Color, 4>> paletteList_ = {
		{
			{ Color{ 232 }, Color{ 160 }, Color{ 88 }, Color{ 16 }, },
			{ Color{ 224,248,208 }, Color{ 136,192,112 }, Color{ 52,104,86 }, Color{ 8,24,32 }, },
			{ Color{ 134,163,90 }, Color{ 111,137,79 }, Color{ 88,117,79 }, Color{ 50,84,79 }, },
			{ Color{ 255,239,206 }, Color{ 222,148,74 }, Color{ 173,41,33 }, Color{ 49,24,82 }, },
		}
	};

	// 現在の画面表示用パレット番号
	int currentPalette_ = 0;

	// デバッグ用モニタを表示する
	bool showDebugMonitor_ = true;

	// メモリダンプするアドレス設定用
	uint16 dumpAddress_ = 0;

};

void Main()
{
	auto app = std::make_unique<DmgeApp>();
	app->run();
}
