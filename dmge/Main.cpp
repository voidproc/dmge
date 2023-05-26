# include <Siv3D.hpp> // OpenSiv3D v0.6.9

#include "Memory.h"
#include "CPU.h"
#include "PPU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Address.h"
#include "AppConfig.h"


void LoadAssets()
{
	FontAsset::Register(U"debug", 8, U"fonts/PressStart2P-Regular.ttf", FontStyle::Bitmap);
	FontAsset::Load(U"debug", U"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ=-~^|@`[{;+:*]},<.>/?_");
}

void InitScene(int scale)
{
	const auto SceneSize = Size{ 160, 144 } * scale;
	Scene::Resize(SceneSize);
	Window::Resize(SceneSize);

	Scene::SetBackground(Palette::Whitesmoke);

	Scene::SetTextureFilter(TextureFilter::Nearest);

	Graphics::SetVSyncEnabled(true);
}

void DrawStatusText(StringView text)
{
	const Size size{ 8 * text.length(), 8 };
	const Rect region{ Scene::Rect().tr().movedBy(-size.x, 1), size };
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
			Console.open();
		}

		config_.print();

		// フォントなどのアセット読み込み
		LoadAssets();

		// Siv3Dのシーン・ウィンドウなどの初期化
		InitScene(config_.scale);

		// 画面表示用パレット初期化
		setPPUPalette_(0);

	}

	void run()
	{
		// カートリッジが指定されていない：
		if (not FileSystem::Exists(config_.cartridgePath))
		{
			Console.writeln(U"Cartridge not found");
			return;
		}

		// カートリッジを読み込み
		mem_.init(&ppu_, &timer_, &joypad_);
		mem_.loadCartridge(config_.cartridgePath);
		mem_.dumpCartridgeInfo();

		// メモリ書き込み時フック
		if (config_.enableBreakpoint && not config_.breakpointsMemWrite.empty())
		{
			mem_.setWriteHook([&](uint16 addr, uint8 value) {
				onMemoryWrite_(addr, value);
			});
		}

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

			if (reachedBreakpoint())
			{
				mode_ = Mode::Trace;
				Console.writeln(U"Break: pc={:04X}"_fmt(cpu_.currentPC()));
			}

			// トレースモードなら、専用のループへ

			if (mode_ == Mode::Trace)
			{
				cpu_.dump();

				traceLoop_();
			}

			// CPUコマンドを1回実行する

			cpu_.applyScheduledIME();
			cpu_.run();

			cyclesFromPreviousDraw += cpu_.consumedCycles();

			// タイマーを更新

			for (int i : step(cpu_.consumedCycles()))
			{
				timer_.update();
			}

			// PPU

			for (int i : step(cpu_.consumedCycles()))
			{
				ppu_.run();

				// VBlankに移行した時のみ描画する
				if (ppu_.modeChangedToVBlank())
				{
					toDraw = true;
				}
			}

			// 割り込み

			cpu_.interrupt();

			// 描画

			if (toDraw || cyclesFromPreviousDraw > 70224 + 16)
			{
				mainDraw_();

				toDraw = false;
				cyclesFromPreviousDraw = 0;
			}
		}
	}

	void mainDraw_()
	{
		if (not System::Update())
		{
			quitApp_ = true;
			return;
		}

		// ステップ実行に移行
		if (KeyP.down())
		{
			mode_ = Mode::Trace;
		}

		// パレット切り替え
		if (KeyC.down())
		{
			currentPalette_ = (currentPalette_ + 1) % paletteList_.size();
			setPPUPalette_(currentPalette_);
		}

		// ボタン入力の更新
		joypad_.update();

		// PPUのレンダリング結果を画面表示
		ppu_.draw(Point{ 0, 0 }, config_.scale);

		if (config_.showFPS)
		{
			DrawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
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

			DrawStatusText(U"TRACE");
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
				Console.writeln(U"Break(Memory Write): pc={:04X} mem={:04X} val={:02X}"_fmt(cpu_.currentPC(), addr, value));
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
	bool reachedBreakpoint()
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

};

void Main()
{
	DmgeApp app{};
	app.run();
}
