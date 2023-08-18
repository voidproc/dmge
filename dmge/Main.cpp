#include "stdafx.h"
#include "App.h"
#include "AppConfig.h"
#include "DebugPrint.h"
#include "DebugMonitor.h"
#include "Version.h"

namespace
{
	void LoadAssets()
	{
		const auto preloadText = U"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ=-~^|@`[{;+:*]},<.>/?_";

#if SIV3D_PLATFORM(WINDOWS)
		FontAsset::Register(U"debug", 10, Resource(U"fonts/JF-Dot-MPlus10.ttf"), FontStyle::Bitmap);
		FontAsset::Register(U"textbox", 12, Resource(U"fonts/JF-Dot-MPlus12.ttf"), FontStyle::Bitmap);
		FontAsset::Register(U"menu", 12, Resource(U"fonts/JF-Dot-MPlus12.ttf"), FontStyle::Bitmap);
#elif SIV3D_PLATFORM(WEB)
		FontAsset::Register(U"debug", 10, U"/fonts/JF-Dot-MPlus10-subset.ttf", FontStyle::Bitmap);
		FontAsset::Register(U"textbox", 12, U"/fonts/JF-Dot-MPlus12-subset.ttf", FontStyle::Bitmap);
		FontAsset::Register(U"menu", 12, U"/fonts/JF-Dot-MPlus12-subset.ttf", FontStyle::Bitmap);
#endif

		FontAsset::Load(U"debug", preloadText);
		FontAsset::Load(U"textbox", preloadText);
		FontAsset::Load(U"menu", preloadText);
	}

	void InitScene()
	{
		Window::SetTitle(U"dmge {}"_fmt(dmge::Version));

		Scene::SetBackground(dmge::DebugMonitor::BgColor);

		Scene::SetLetterbox(Palette::Black);

		Scene::SetTextureFilter(TextureFilter::Nearest);

		Graphics::SetVSyncEnabled(true);

		System::SetTerminationTriggers(UserAction::CloseButtonClicked);

		Profiler::EnableAssetCreationWarning(false);
	}

#if SIV3D_PLATFORM(WINDOWS)
	void SaveScreenshot(FilePathView path)
	{
		ScreenCapture::SaveCurrentFrame(path + U"");
		System::Update();
	}
#endif
}

#if SIV3D_PLATFORM(WINDOWS)
void runTest(dmge::AppConfig& config)
{
	config.cartridgePath.clear();
	config.detectCGB = true;
	config.detectSGB = true;
	config.bootROMPath.clear();
	config.scale = 2;
	config.showFPS = false;
	config.palettePreset = 1;
	config.cgbColorGamma = 1.0;
	config.showDebugMonitor = false;
	config.showConsole = true;
	config.breakpoints.clear();
	config.memoryWriteBreakpoints.clear();
	config.breakOnLDBB = false;
	config.enableBreakpoint = false;
	config.dumpAddress.clear();
	config.traceDumpStartAddress.clear();
	config.logFilePath.clear();

	dmge::DebugPrint::EnableConsole();

	LoadAssets();

	InitScene();

	TextWriter writer{ U"mooneye_test_result.csv" };

	const FilePath ssDirName = U"mooneye_test_result_{}"_fmt(DateTime::Now().format(U"yyyyMMdd_HHmmss"));

	const auto mooneyeTestRoms = FileSystem::DirectoryContents(U"cartridges/test/mooneye", Recursive::Yes);

	for (const auto& testRomPath : mooneyeTestRoms)
	{
		if (FileSystem::Extension(testRomPath) != U"gb") continue;

		auto app = std::make_unique<dmge::DmgeApp>(config);

		app->setCartridgePath(testRomPath);

		app->run();

		SaveScreenshot(U"{}/{}.png"_fmt(ssDirName, FileSystem::BaseName(testRomPath)));

		writer.writeln(U"{},{}"_fmt(testRomPath, app->mooneyeTestResult() == dmge::MooneyeTestResult::Passed ? 1 : 0));
	}
}
#endif

void Main()
{
#if SIV3D_PLATFORM(WINDOWS)
	dmge::AppConfig config = dmge::AppConfig::LoadConfig();
#elif SIV3D_PLATFORM(WEB)
	dmge::AppConfig config;
#endif

#if SIV3D_PLATFORM(WINDOWS)
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
#endif

	// フォントなどのアセット読み込み
	LoadAssets();

	// Siv3Dのシーン・ウィンドウなどの初期化
	InitScene();

#if SIV3D_PLATFORM(WEB)
	while (System::Update())
	{
		FontAsset(U"menu")(U"Click here to start emulation...").drawAt(Scene::Center());

		if (Scene::Rect().leftClicked())
		{
			break;
		}
	}

	config.paletteColors[0] = ColorF{ U"#E0E9C4" };
	config.paletteColors[1] = ColorF{ U"#9AA57C" };
	config.paletteColors[2] = ColorF{ U"#4B564D" };
	config.paletteColors[3] = ColorF{ U"#252525" };

	config.cartridgePath = U"/cartridges/pocket.gb";
#endif

	Optional<String> cartridgePath = config.cartridgePath;

	while (true)
	{
		auto app = std::make_unique<dmge::DmgeApp>(config);

		if (cartridgePath)
		{
			app->setCartridgePath(*cartridgePath);
		}

		app->run();

		const auto mode = app->mode();

		// リセットされていない場合はアプリケーションを終了
		if (mode != dmge::DmgeAppMode::Reset) break;

		// 違うファイルが選択されているかもしれないので読み込む
		cartridgePath = app->currentCartridgePath();
	}

#if SIV3D_PLATFORM(WINDOWS)
	config.save();
#endif
}
