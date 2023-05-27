# include <Siv3D.hpp> // OpenSiv3D v0.6.9

#include "Memory.h"
#include "CPU.h"
#include "PPU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Address.h"
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

		config_.print();

		mem_.init(&ppu_, &timer_, &joypad_);

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
		// カートリッジが指定されていない：
		if (not FileSystem::Exists(config_.cartridgePath))
		{
			dmge::DebugPrint::Writeln(U"Cartridge not found");
			return;
		}

		// カートリッジを読み込み
		mem_.loadCartridge(config_.cartridgePath);
		mem_.dumpCartridgeInfo();


		mainLoop_();
	}

private:

	void mainLoop_()
	{
		// VBlankに変化したら描画する
		bool toDraw = false;

		// 前回の描画からの経過サイクル数
		// ※基本的に描画タイミングはVBlank移行時だが、LCD=offの場合にはVBlankに移行しないので
		//   画面の更新（System::Update()）がされない期間が長くなってしまうため、
		//   経過サイクル数が一定数を超えた場合にも描画を行う
		int cyclesFromPreviousDraw = 0;

		while (not quitApp_)
		{
			// ブレークポイントが有効で、ブレークポイントに達していたら
			// トレースモードに切り替える

			if (reachedBreakpoint_())
			{
				mode_ = Mode::Trace;
				dmge::DebugPrint::Writeln(U"Break: pc={:04X}"_fmt(cpu_.currentPC()));
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

			// 描画されないまま一定のサイクル数が経過した場合に強制的に描画する
			cyclesFromPreviousDraw += cpu_.consumedCycles();
			if (cyclesFromPreviousDraw > 70224 + 16)
			{
				toDraw = true;
			}

			// タイマーを更新

			for (int i : step(cpu_.consumedCycles()))
			{
				timer_.update();
			}

			// PPU

			for (int i : step(cpu_.consumedCycles()))
			{
				ppu_.run();

				if (ppu_.modeChangedToVBlank())
				{
					toDraw = true;
				}
			}

			// 割り込み

			cpu_.interrupt();

			// キー入力と描画

			if (toDraw)
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
				}

				// ボタン入力の更新
				// ※デバッグモニタのテキストボックス入力中は更新しない
				if (not (showDebugMonitor_ && debugMonitor_.isVisibleTextbox()))
				{
					joypad_.update();
				}

				// PPUのレンダリング結果を画面表示
				ppu_.draw(Point{ 0, 0 }, config_.scale);

				// デバッグ用モニタ表示
				if (showDebugMonitor_)
				{
					debugMonitor_.draw(Point{ 160 * config_.scale, 0 });
				}

				if (config_.showFPS)
				{
					DrawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
				}

				toDraw = false;
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

	dmge::Timer timer_{ &mem_ };

	dmge::CPU cpu_{ &mem_ };

	dmge::Joypad joypad_{ &mem_ };

	dmge::DebugMonitor debugMonitor_{ &mem_, &cpu_ };

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
	DmgeApp app{};
	app.run();
}
