# include <Siv3D.hpp> // OpenSiv3D v0.6.10

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

		FontAsset::Register(U"debug", 10, Resource(U"fonts/JF-Dot-MPlus10.ttf"), FontStyle::Bitmap);
		FontAsset::Register(U"menu", 12, Resource(U"fonts/JF-Dot-MPlus12.ttf"), FontStyle::Bitmap);

		FontAsset::Load(U"debug", preloadText);
		FontAsset::Load(U"menu", preloadText);
	}

	void InitScene()
	{
		Window::SetTitle(U"dmge {}"_fmt(dmge::Version));

		Scene::SetBackground(dmge::DebugMonitor::BgColor);

		Scene::SetTextureFilter(TextureFilter::Nearest);

		Graphics::SetVSyncEnabled(true);

		System::SetTerminationTriggers(UserAction::CloseButtonClicked);

		Profiler::EnableAssetCreationWarning(false);
	}

	void SaveScreenshot(FilePathView path)
	{
		ScreenCapture::SaveCurrentFrame(path + U"");
		System::Update();
	}
}

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
	InitScene();


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
}
