﻿# include <Siv3D.hpp> // OpenSiv3D v0.6.10

#include "Memory.h"
#include "Cartridge.h"
#include "CPU.h"
#include "PPU.h"
#include "LCD.h"
#include "Audio/APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Serial.h"
#include "SGB/Command.h"
#include "Interrupt.h"
#include "Address.h"
#include "Timing.h"
#include "AppConfig.h"
#include "DebugPrint.h"
#include "DebugMonitor.h"
#include "Version.h"


namespace
{
	void LoadAssets()
	{
		const auto preloadText = U"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ=-~^|@`[{;+:*]},<.>/?_";

		FontAsset::Register(U"debug", 10, Resource(U"fonts/JF-Dot-MPlus10.ttf"), FontStyle::Bitmap);
		FontAsset::Load(U"debug", preloadText);
	}

	void SetWindowSize(int scale, bool showDebugMonitor)
	{
		const int width = 160 * scale;
		const int height = 144 * scale;

		const int widthWithDebugMonitor = width + dmge::DebugMonitor::ViewportSize.x;
		const int heightWithDebugMonitor = Max(height, dmge::DebugMonitor::ViewportSize.y);

		const auto SceneSize = showDebugMonitor ? Size{ widthWithDebugMonitor, heightWithDebugMonitor } : Size{ width, height };
		Scene::Resize(SceneSize);
		Window::Resize(SceneSize);
	}

	void InitScene(int scale, bool showDebugMonitor)
	{
		SetWindowSize(scale, showDebugMonitor);

		Window::SetTitle(U"dmge {}"_fmt(dmge::Version));

		Scene::SetBackground(dmge::DebugMonitor::BgColor);

		Scene::SetTextureFilter(TextureFilter::Nearest);

		Graphics::SetVSyncEnabled(true);

		System::SetTerminationTriggers(UserAction::CloseButtonClicked);

		Profiler::EnableAssetCreationWarning(false);
	}

	void DrawStatusText(StringView text)
	{
		const Size size{ 5 * text.length(), 10 };
		const Rect region{ Scene::Rect().tr().movedBy(-size.x, 0), size };
		region.stretched(1, 1).draw(Color{ 0, 128 });
		FontAsset(U"debug")(text).draw(region.pos);
	}

	Optional<String> ChooseCartridge(FilePathView defaultDirectory)
	{
		const auto directory = FileSystem::FullPath(defaultDirectory);
		return Dialog::OpenFile({ FileFilter{ .name = U"GAMEBOY Cartridge", .patterns = {U"gb?"} } }, directory, U"ファイルを開く");
	}

	void SaveScreenshot(FilePathView path)
	{
		ScreenCapture::SaveCurrentFrame(path + U"");
		System::Update();
	}
}


enum class DmgeAppMode
{
	Default,
	Trace,
	Reset,
};

class DmgeApp
{
public:
	DmgeApp(dmge::AppConfig& config)
	{
		config_ = config;

		showDebugMonitor_ = config_.showDebugMonitor;

		mem_.init(&ppu_, &apu_, &timer_, &joypad_, &lcd_, &interrupt_, &serial_);

		joypad_.setButtonAssign(config_.gamepadButtonAssign);

		// メモリ書き込み時フックを設定
		if (config_.enableBreakpoint && not config_.memoryWriteBreakpoints.empty())
		{
			mem_.setWriteHook([&](uint16 addr, uint8 value) {
				onMemoryWrite_(addr, value);
			});
		}

		// 画面表示用パレット初期化
		setPPUPalette_(0);

		// ピクセルシェーダ用パラメータ
		ppu_.setGamma(config_.cgbColorGamma);

		// オーディオ LPF
		apu_.setLPFConstant(config_.audioLPFConstant);
		apu_.setEnableLPF(config_.enableAudioLPF);
	}

	void setCartridgePath(const String& cartridgePath)
	{
		currentCartridgePath_ = cartridgePath;
	}

	void run()
	{
		// 設定ファイルでカートリッジが指定されていない場合は
		// ファイルを開くダイアログで選択する
		if (not dmge::IsValidCartridgePath(currentCartridgePath_.value_or(U"")))
		{
			currentCartridgePath_ = ChooseCartridge(config_.openCartridgeDirectory);
		}

		// ファイルが開けない場合は終了する
		if (not dmge::IsValidCartridgePath(currentCartridgePath_.value_or(U"")))
		{
			dmge::DebugPrint::Writeln(U"* Cartridge not found");
			return;
		}

		// カートリッジ読み込み
		// 対応するMBCがカートリッジをロードする
		// 対応するMBCがない場合は終了する
		if (not mem_.loadCartridge(*currentCartridgePath_))
		{
			dmge::DebugPrint::Writeln(U"* Cannot open cartridge");
			return;
		}

		// SRAMをロード
		mem_.loadSRAM();

		// BootROM
		bool enableBootROM = not config_.bootROMPath.isEmpty() && FileSystem::Exists(config_.bootROMPath);
		if (enableBootROM)
		{
			mem_.enableBootROM(config_.bootROMPath);
		}

		// [DEBUG]
		dmge::DebugPrint::Writeln(U"* Cartridge loaded:");
		dmge::DebugPrint::Writeln(U"FileName={}"_fmt(FileSystem::FileName(*currentCartridgePath_)));
		mem_.dumpCartridgeInfo();

		// CGBモードの適用
		if (mem_.isSupportedCGBMode())
		{
			mem_.setCGBMode(true);
			ppu_.setCGBMode(true);
			apu_.setCGBMode(true);
			cpu_.setCGBMode(true);
			debugMonitor_.setCGBMode(true);
		}

		cpu_.reset(enableBootROM);

		// SGBモードの適用
		if (not mem_.isCGBMode() && mem_.isSupportedSGBMode())
		{
			mem_.setSGBMode(true);
			ppu_.setSGBMode(true);
			cpu_.setSGBMode(true);
		}

		// メモリの内容をリセット（SGB/CGBモード確定後にリセットする必要がある）
		mem_.reset();

		mainLoop_();

		// アプリケーション終了時にSRAMを保存する
		mem_.saveSRAM();
	}

	String currentCartridgePath() const
	{
		return currentCartridgePath_.value_or(U"");
	}

	DmgeAppMode mode() const
	{
		return mode_;
	}

	dmge::MooneyeTestResult mooneyeTestResult() const
	{
		return mooneyeTestResult_;
	}

private:

	void mainLoop_()
	{
		dmge::FPSKeeper fpsKeeper{};

		// モニタのリフレッシュレートが 60Hz の場合は FPS 制御を Siv3D に任せる
		// そうでない場合は FPSKeeper で 60[FPS] を保つ
		if (System::GetCurrentMonitor().refreshRate == 60)
		{
			fpsKeeper.setEnable(false);
		}

		while (not quitApp_)
		{
			// ブレークポイントに達したらトレースモードに切り替える

			if (reachedBreakpoint_())
			{
				mode_ = DmgeAppMode::Trace;

				apu_.pause();

				dmge::DebugPrint::Writeln(U"Break: pc={:04X}"_fmt(cpu_.currentPC()));
			}

			// トレースダンプを開始するアドレスに達していたら
			// トレースダンプ開始

			if (reachedTraceDumpAddress_())
			{
				enableTraceDump_ = true;
			}

			// トレースモードならメモリ・CPUをダンプ
			// トレースモード専用のループへ

			if (mode_ == DmgeAppMode::Trace)
			{
				for (const uint16 addr : config_.dumpAddress)
				{
					mem_.dump(addr, addr + 15);
				}

				cpu_.dump();

				traceLoop_();
			}
			else
			{
				if (enableTraceDump_)
				{
					//dmge::DebugPrint::Writeln(U"dot={}"_fmt(ppu_.dot() % 456));
					cpu_.dump();
				}
			}

			// テストモードの場合、テスト結果をチェック

			if (config_.testMode)
			{
				mooneyeTestResult_ = cpu_.mooneyeTestResult();
				if (mooneyeTestResult_ != dmge::MooneyeTestResult::Running)
				{
					return;
				}
			}

			// CPUコマンドを1回実行する
			cpu_.run();

			tickUnits_(cpu_.consumedCycles());

			// 割り込み
			if (cpu_.interrupt())
			{
				tickUnits_(5 * 4);
			}

			// キー入力と描画

			if (checkShouldDraw_())
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
					mode_ = DmgeAppMode::Trace;

					apu_.pause();
				}

				// PPUのレンダリング結果を画面表示
				ppu_.draw(Vec2{ 0, 0 }, config_.scale);

				// APU
				if (enableAPU_ && mode_ != DmgeAppMode::Trace)
				{
					apu_.playIfBufferEnough(2000);
					apu_.pauseIfBufferNotEnough(512);
				}

				// デバッグ用モニタ表示
				if (showDebugMonitor_)
				{
					updateDebugMonitor_();
				}

				// デバッグモニタのテキストボックス入力中はJOYPADを更新しない
				joypad_.setEnable(not (showDebugMonitor_ && debugMonitor_.isVisibleTextbox()));

				if (config_.showFPS)
				{
					DrawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
				}

				fpsKeeper.sleep();

				cyclesFromPreviousDraw_ = 0;

			}
		}
	}

	void traceLoop_()
	{
		while (mode_ == DmgeAppMode::Trace)
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
				mode_ = DmgeAppMode::Default;
				break;
			}

			ppu_.draw(Point{ 0, 0 }, config_.scale);

			// デバッグ用モニタ表示
			if (showDebugMonitor_)
			{
				updateDebugMonitor_();
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

		// [DEBUG]常にダンプ
		if (KeyD.down())
		{
			enableTraceDump_ = not enableTraceDump_;

			dmge::DebugPrint::Writeln(U"EnableTraceDump={}"_fmt(enableTraceDump_));
		}

		// パレット切り替え
		if (KeyL.down())
		{
			currentPalette_ = (currentPalette_ + 1) % paletteList_.size();
			setPPUPalette_(currentPalette_);
		}

		// リセット (Ctrl+R)
		if (KeyR.down() && KeyControl.pressed())
		{
			mode_ = DmgeAppMode::Reset;
			quitApp_ = true;
		}

		// 開く (Ctrl+O)
		if (KeyO.down() && KeyControl.pressed())
		{
			if (const auto openPath = ChooseCartridge(FileSystem::ParentPath(currentCartridgePath_.value_or(U"")));
				openPath)
			{
				currentCartridgePath_ = openPath;
				mode_ = DmgeAppMode::Reset;
				quitApp_ = true;
			}
		}

		// APUの各チャンネルをミュート

		if (Key1.down())
		{
			apu_.setMute(0, not apu_.getMute(0));
		}

		if (Key2.down())
		{
			apu_.setMute(1, not apu_.getMute(1));
		}

		if (Key3.down())
		{
			apu_.setMute(2, not apu_.getMute(2));
		}

		if (Key4.down())
		{
			apu_.setMute(3, not apu_.getMute(3));
		}

		// APUの使用を切替
		if (Key5.down())
		{
			enableAPU_ = not enableAPU_;
			if (not enableAPU_)
			{
				apu_.pause();
			}
		}

		// Toggle LPF
		if (Key6.down())
		{
			apu_.setEnableLPF(not apu_.getEnableLPF());
		}
	}

	// メモリ書き込み時フック
	void onMemoryWrite_(uint16 addr, uint8 value)
	{
		if (not config_.enableBreakpoint || mode_ == DmgeAppMode::Trace)
		{
			return;
		}

		for (const auto& mem : config_.memoryWriteBreakpoints)
		{
			if (addr == mem)
			{
				mode_ = DmgeAppMode::Trace;

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
	}

	// ブレークポイントが有効かつブレークポイントに達したか
	bool reachedBreakpoint_() const
	{
		if (not config_.enableBreakpoint) return false;

		for (const auto bp : config_.breakpoints)
		{
			if (cpu_.currentPC() == bp)
			{
				return true;
			}
		}

		if (config_.breakOnLDBB && mem_.read(cpu_.currentPC()) == 0x40)
		{
			return true;
		}

		return false;
	}

	bool reachedTraceDumpAddress_() const
	{
		if (not config_.traceDumpStartAddress) return false;

		for (const auto addr : config_.traceDumpStartAddress)
		{
			if (cpu_.currentPC() == addr)
			{
				return true;
			}
		}

		return false;
	}

	void tickUnits_(int cycles)
	{
		int doubleSpeedFactor = mem_.isDoubleSpeed() ? 2 : 1;

		// RTC, DMA
		mem_.update(cycles);

		// タイマーを更新

		for (int i : step(cycles))
		{
			timer_.update();
		}

		// Serial

		for (int i : step(cycles))
		{
			serial_.update();
		}

		// PPU

		for (int i : step(cycles / doubleSpeedFactor))
		{
			ppu_.run();
		}

		// APU

		if (enableAPU_)
		{
			for (int i : step(cycles / doubleSpeedFactor))
			{
				bufferedSamples_ += apu_.run();
			}
		}
	}

	bool checkShouldDraw_()
	{
		bool shouldDraw = false;

		// (1) オーディオバッファに1フレーム分のサンプルを書き込んだら描画する

		int doubleSpeedFactor = mem_.isDoubleSpeed() ? 2 : 1;

		if (enableAPU_)
		{
			if (bufferedSamples_ > 1.0 * sampleRate_ / 60.0 * doubleSpeedFactor)
			{
				bufferedSamples_ -= sampleRate_ / 60.0 * doubleSpeedFactor;

				shouldDraw = true;
			}
		}

		// (2) 描画されないまま一定のサイクル数が経過した場合に強制的に描画する

		cyclesFromPreviousDraw_ += cpu_.consumedCycles();

		if (cyclesFromPreviousDraw_ >= dmge::ClockFrequency / 59.50 * doubleSpeedFactor)
		{
			shouldDraw = true;
		}

		return shouldDraw;
	}

	void updateDebugMonitor_()
	{
		debugMonitor_.update();

		debugMonitor_.draw(Point{ 160 * config_.scale, 0 });

		debugMonitor_.updateGUI();
	}


	dmge::AppConfig config_;

	dmge::Memory mem_{};

	dmge::Interrupt interrupt_{};

	dmge::LCD lcd_{ mem_ };

	dmge::PPU ppu_{ &mem_, &lcd_, &interrupt_ };

	// (APU)サンプリングレート
	int sampleRate_ = 44100;

	dmge::APU apu_{ timer_, sampleRate_ };

	dmge::Timer timer_{ &mem_, &interrupt_ };

	dmge::CPU cpu_{ &mem_, &interrupt_ };

	dmge::Joypad joypad_{ &mem_ };

	dmge::Serial serial_{ interrupt_ };

	dmge::DebugMonitor debugMonitor_{ &mem_, &cpu_, &apu_, &interrupt_ };

	// モード（通常 or トレース）
	DmgeAppMode mode_ = DmgeAppMode::Default;

	// アプリケーションを終了する
	bool quitApp_ = false;

	// 画面表示用パレット
	const Array<std::array<ColorF, 4>> paletteList_ = {
		{
			// BGB grey
			{ ColorF{ U"#e8e8e8" }, ColorF{ U"#a0a0a0" }, ColorF{ U"#585858" }, ColorF{ U"#101010" }, },

			// BGB lcd green
			{ ColorF{ U"#e0f8d0" }, ColorF{ U"#88c070" }, ColorF{ U"#346856" }, ColorF{ U"#081820"}, },

			// BGB super gameboy
			{ ColorF{ U"#ffefce" }, ColorF{ U"#de944a" }, ColorF{ U"#ad2921" }, ColorF{ U"#311852" }, },

			// GBP-NSO
			{ ColorF{ U"#b5c69c" }, ColorF{ U"#8d9c7b" }, ColorF{ U"#637251" }, ColorF{ U"#303820" }, },

			// DMG-NSO
			{ ColorF{ U"#8cad28" }, ColorF{ U"#6c9421" }, ColorF{ U"#426b29" }, ColorF{ U"#214231" }, },

			// SGB 4-H
			{ ColorF{ U"#f8f8c8" }, ColorF{ U"#b8c058" }, ColorF{ U"#808840" }, ColorF{ U"#405028" }, },
		}
	};

	// APUを使用する
	bool enableAPU_ = true;

	// 現在の画面表示用パレット番号
	int currentPalette_ = 0;

	// デバッグ用モニタを表示する
	bool showDebugMonitor_ = true;

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

	dmge::MooneyeTestResult mooneyeTestResult_ = dmge::MooneyeTestResult::Running;

};

void runTest(dmge::AppConfig& config)
{
	config.bootROMPath.clear();
	config.breakOnLDBB = false;
	config.breakpoints.clear();
	config.cartridgePath.clear();
	config.dumpAddress.clear();
	config.logFilePath.clear();
	config.memoryWriteBreakpoints.clear();
	config.showConsole = true;
	config.showDebugMonitor = false;
	config.showFPS = false;
	config.traceDumpStartAddress.clear();
	config.scale = 2;

	dmge::DebugPrint::EnableConsole();

	LoadAssets();

	InitScene(config.scale, config.showDebugMonitor);

	TextWriter writer{ U"mooneye_test_result.csv" };

	const FilePath ssDirName = U"mooneye_test_result_{}"_fmt(DateTime::Now().format(U"yyyyMMdd_HHmmss"));

	const auto mooneyeTestRoms = FileSystem::DirectoryContents(U"cartridges/test/mooneye", Recursive::Yes);

	for (const auto& testRomPath : mooneyeTestRoms)
	{
		if (FileSystem::Extension(testRomPath) != U"gb") continue;

		auto app = std::make_unique<DmgeApp>(config);

		app->setCartridgePath(testRomPath);

		app->run();

		SaveScreenshot(U"{}/{}.png"_fmt(ssDirName, FileSystem::BaseName(testRomPath)));

		writer.writeln(U"{},{}"_fmt(testRomPath, app->mooneyeTestResult() == dmge::MooneyeTestResult::Passed ? 1 : 0));
	}
}

void Main()
{
	dmge::AppConfig config = dmge::AppConfig::LoadConfig();

	if (config.testMode)
	{
		runTest(config);
		return;
	}

	if (config.showConsole)
	{
		dmge::DebugPrint::EnableConsole();
	}

	if (not config.logFilePath.isEmpty())
	{
		dmge::DebugPrint::EnableFileOutput(config.logFilePath);
	}

	// [DEBUG]
	config.print();

	// フォントなどのアセット読み込み
	LoadAssets();

	// Siv3Dのシーン・ウィンドウなどの初期化
	InitScene(config.scale, config.showDebugMonitor);


	Optional<String> cartridgePath = config.cartridgePath;

	while (true)
	{
		auto app = std::make_unique<DmgeApp>(config);

		if (cartridgePath)
		{
			app->setCartridgePath(*cartridgePath);
		}

		app->run();

		const auto mode = app->mode();
		if (mode != DmgeAppMode::Reset) break;

		cartridgePath = app->currentCartridgePath();
	}
}
