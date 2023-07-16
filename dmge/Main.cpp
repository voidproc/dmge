# include <Siv3D.hpp> // OpenSiv3D v0.6.10

#include "Memory.h"
#include "Cartridge.h"
#include "CPU.h"
#include "PPU.h"
#include "LCD.h"
#include "Audio/APU.h"
#include "Timer.h"
#include "Joypad.h"
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

		mem_.init(&ppu_, &apu_, &timer_, &joypad_, &lcd_, &interrupt_);

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
		ppu_.setCGBMode(mem_.isCGBMode());
		apu_.setCGBMode(mem_.isCGBMode());
		cpu_.setCGBMode(mem_.isCGBMode());
		cpu_.reset(enableBootROM);

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
		// RTC, DMA
		mem_.update(cycles);

		// タイマーを更新

		for (int i : step(cycles))
		{
			timer_.update();
		}

		// PPU

		for (int i : step(cycles))
		{
			ppu_.run();
		}

		// APU

		if (enableAPU_)
		{
			for (int i : step(cycles))
			{
				bufferedSamples_ += apu_.run();
			}
		}
	}

	bool checkShouldDraw_()
	{
		bool shouldDraw = false;

		// (1) オーディオバッファに1フレーム分のサンプルを書き込んだら描画する

		if (enableAPU_)
		{
			if (bufferedSamples_ > 1.0 * sampleRate_ / 60.0)
			{
				bufferedSamples_ -= sampleRate_ / 60.0;

				shouldDraw = true;
			}
		}

		// (2) 描画されないまま一定のサイクル数が経過した場合に強制的に描画する

		cyclesFromPreviousDraw_ += cpu_.consumedCycles();

		if (cyclesFromPreviousDraw_ >= dmge::ClockFrequency / 59.50)
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

	dmge::LCD lcd_{};

	dmge::PPU ppu_{ &mem_, &lcd_, &interrupt_ };

	// (APU)サンプリングレート
	int sampleRate_ = 44100;

	dmge::APU apu_{ &mem_, sampleRate_ };

	dmge::Timer timer_{ &mem_, &interrupt_ };

	dmge::CPU cpu_{ &mem_, &interrupt_ };

	dmge::Joypad joypad_{ &mem_ };

	dmge::DebugMonitor debugMonitor_{ &mem_, &cpu_, &apu_, &interrupt_ };

	// モード（通常 or トレース）
	DmgeAppMode mode_ = DmgeAppMode::Default;

	// アプリケーションを終了する
	bool quitApp_ = false;

	// 画面表示用パレット
	const Array<std::array<Color, 4>> paletteList_ = {
		{
			// BGB grey
			{ Color{ 232 }, Color{ 160 }, Color{ 88 }, Color{ 16 }, },

			// BGB lcd green
			{ Color{ 224,248,208 }, Color{ 136,192,112 }, Color{ 52,104,86 }, Color{ 8,24,32 }, },

			// BGB super gameboy
			{ Color{ 255,239,206 }, Color{ 222,148,74 }, Color{ 173,41,33 }, Color{ 49,24,82 }, },

			// GBP-NSO
			{ Color{ U"#b5c69c" }, Color{ U"#8d9c7b" }, Color{ U"#637251" }, Color{ U"#303820" },  },

			// DMG-NSO
			{ Color{ U"#8cad28" }, Color{ U"#6c9421" }, Color{ U"#426b29" }, Color{ U"#214231" },  },

			// SGB 4-H
			{ Color{ U"#f8f8c8" }, Color{ U"#b8c058" }, Color{ U"#808840" }, Color{ U"#405028" },  },
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


};

void Main()
{
	dmge::AppConfig config = dmge::AppConfig::LoadConfig();

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
