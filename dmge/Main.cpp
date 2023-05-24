# include <Siv3D.hpp> // OpenSiv3D v0.6.9

#include "Memory.h"
#include "CPU.h"
#include "PPU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Address.h"
#include "AppConfig.h"


void loadAssets()
{
	FontAsset::Register(U"debug", 8, U"fonts/PressStart2P-Regular.ttf", FontStyle::Bitmap);
	FontAsset::Load(U"debug", U"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ=-~^|@`[{;+:*]},<.>/?_");
}

void drawStatusText(StringView text)
{
	const Size size{ 8 * text.length(), 8 };
	const Rect region{ Scene::Rect().tr().movedBy(-size.x, 1), size };
	region.stretched(1, 1).draw(Color{0, 128});
	FontAsset(U"debug")(text).draw(region.pos);
}

void Main()
{
	Console.open();
	Console.writeln(U"Setup ...");

	dmge::AppConfig config = dmge::AppConfig::LoadConfig();
	config.print();

	if (not FileSystem::Exists(config.cartridgePath))
	{
		Console.writeln(U"Cartridge not found");
		return;
	}

	const auto SceneSize = Size{ 160, 144 } * config.scale;

	Scene::Resize(SceneSize);
	Window::Resize(SceneSize);
	Scene::SetBackground(Palette::Whitesmoke);
	Scene::SetTextureFilter(TextureFilter::Nearest);
	const ScopedRenderStates2D renderState{ SamplerState::ClampNearest };

	loadAssets();

	dmge::Memory mem;

	dmge::PPU ppu{ &mem };

	dmge::Timer timer{ &mem };

	dmge::CPU cpu{ &mem };

	dmge::Joypad joypad{ &mem };

	mem.init(&ppu, &timer, &joypad);
	mem.loadCartridge(config.cartridgePath);


	// ---- Wait ----

	Console.writeln(U"Wait ...");

	Graphics::SetVSyncEnabled(true);

	while (System::Update())
	{
		if (Scene::Time() > 0.5)
		{
			break;
		}
	}


	// ---- Start emulation ----

	// メインループのループ回数
	int counter = 0;

	// トレース中
	bool trace = false;

	// VBlankに変化したので描画する
	bool toDraw = false;

	int cyclesFromPreviousDraw = 0;

	// アプリケーションを終了する
	bool quitApp = false;

	// パレット
	const Array<std::array<Color, 4>> paletteList = {
		{
			{ Color{ 232 }, Color{ 160 }, Color{ 88 }, Color{ 16 }, },
			{ Color{ 224,248,208 }, Color{ 136,192,112 }, Color{ 52,104,86 }, Color{ 8,24,32 }, },
			{ Color{ 134,163,90 }, Color{ 111,137,79 }, Color{ 88,117,79 }, Color{ 50,84,79 }, },
			{ Color{ 255,239,206 }, Color{ 222,148,74 }, Color{ 173,41,33 }, Color{ 49,24,82 }, },
		}
	};

	int paletteIndex = 0;
	ppu.setDisplayColorPalette(paletteList[0]);
	Scene::SetBackground(paletteList[0][0]);


	// メモリ書き込み時にブレーク

	if (config.enableBreakpoint)
	{
		mem.setWriteHook([&](uint16 addr, uint8 value) {
			if (not config.enableBreakpoint)
			{
				return;
			}

			for (const auto& mem : config.breakpointsMemWrite)
			{
				if (addr == mem)
				{
					trace = true;
					Console.writeln(U"Break(Memory Write): pc={:04X} mem={:04X} val={:02X}"_fmt(cpu.currentPC(), addr, value));
					cpu.dump();
					break;
				}
			}
		});
	}


	Console.writeln(U"Start main loop");

	while (not quitApp)
	{
		// ブレークポイントに到達したらトレースモードに移行

		if (config.enableBreakpoint)
		{
			for (const auto bp : config.breakpoints)
			{
				if (cpu.currentPC() == bp)
				{
					trace = true;
					Console.writeln(U"Break: pc={:04X}"_fmt(cpu.currentPC()));
					break;
				}
			}
		}

		// トレース中、次に実行する命令とレジスタの内容などを出力する

		if (trace)
		{
			cpu.dump();
		}

		while (trace)
		{
			if (not System::Update())
			{
				quitApp = true;
				break;
			}

			if (KeyF7.down())
			{
				// ステップ実行
				break;
			}
			else if (KeyF5.down())
			{
				// トレースモード終了
				trace = false;
				break;
			}

			drawStatusText(U"TRACE");
		}

		// CPUコマンドを1回実行する

		cpu.applyScheduledIME();
		cpu.run();

		// タイマーを更新

		for (int i : step(cpu.consumedCycles()))
		{
			timer.update();
		}

		// PPU

		for (int i : step(cpu.consumedCycles()))
		{
			ppu.run();

			// VBlankに移行した時のみ描画する
			// TOOD: LCDがoffになっている場合はVBlankにならないので、他の方法でタイミングを取って描画する必要がある
			if (ppu.modeChangedToVBlank())
			{
				toDraw = true;
			}
		}

		cyclesFromPreviousDraw += cpu.consumedCycles();

		// 割り込み

		cpu.interrupt();

		// 描画

		if (toDraw || cyclesFromPreviousDraw > 70224 + 16)
		{
			if (not System::Update())
			{
				break;
			}

			// ステップ実行に移行
			if (KeyP.down())
			{
				trace = true;
			}

			// パレット切り替え
			if (KeyC.down())
			{
				paletteIndex = (paletteIndex + 1) % paletteList.size();
				ppu.setDisplayColorPalette(paletteList[paletteIndex]);
				Scene::SetBackground(paletteList[paletteIndex][0]);
			}

			joypad.update();

			ppu.draw(Point{ 0, 0 }, config.scale);

			if (config.showFPS)
			{
				drawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
			}

			toDraw = false;
			cyclesFromPreviousDraw = 0;
		}

		++counter;
	}
}
