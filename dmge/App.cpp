#include "App.h"
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
#include "AppWindow.h"
#include "Colors.h"
#include "InputMapping.h"
#include "InputMappingOverlay.h"

namespace dmge
{
	namespace
	{
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
			return Dialog::OpenFile({ FileFilter{.name = U"GAMEBOY Cartridge", .patterns = {U"gb?"} } }, directory, U"ファイルを開く");
		}
	}

	DmgeApp::DmgeApp(AppConfig& config)
		:
		config_{ config },
		mem_{ std::make_unique<Memory>() },
		interrupt_{ std::make_unique<Interrupt>() },
		lcd_{ std::make_unique<LCD>(*mem_.get()) },
		ppu_{ std::make_unique<PPU>(mem_.get(), lcd_.get(), interrupt_.get()) },
		timer_{ std::make_unique<Timer>(mem_.get(), interrupt_.get()) },
		apu_{ std::make_unique<APU>(*timer_.get()) },
		cpu_{ std::make_unique<CPU>(mem_.get(), interrupt_.get()) },
		joypad_{ std::make_unique<Joypad>(mem_.get()) },
		serial_{ std::make_unique<Serial>(*interrupt_.get()) },
		debugMonitor_{ std::make_unique<DebugMonitor>(mem_.get(), cpu_.get(), apu_.get(), interrupt_.get()) },
		keyMap_{ std::make_unique<InputMapping>(InputDeviceType::Keyboard) },
		gamepadMap_{ std::make_unique<InputMapping>(InputDeviceType::Gamepad) }
	{
		mem_->init(ppu_.get(), apu_.get(), timer_.get(), joypad_.get(), lcd_.get(), interrupt_.get(), serial_.get());

		// config.ini のキー／ボタンマッピングを適用

		keyMap_->set(config_.keyMapping);
		gamepadMap_->set(config_.gamepadMapping);

		joypad_->setMapping(*keyMap_, *gamepadMap_);

		// メモリ書き込み時フックを設定
		if (config_.enableBreakpoint && not config_.memoryWriteBreakpoints.empty())
		{
			mem_->setWriteHook([&](uint16 addr, uint8 value) {
				onMemoryWrite_(addr, value);
			});
		}

		// 画面表示用パレット初期化 (DMG)
		setPPUPalette_(config_.palettePreset);

		// ピクセルシェーダ用パラメータ (CGB)
		ppu_->setGamma(config_.cgbColorGamma);

		// オーディオ LPF
		apu_->setLPFConstant(config_.audioLPFConstant);
		apu_->setEnableLPF(config_.enableAudioLPF);

		// メニュー初期化
		initMenu_();
	}

	DmgeApp::~DmgeApp()
	{
	}

	void DmgeApp::run()
	{
		// ウィンドウサイズを設定
		static bool isFirstResize = true;
		SetScaleWindowSize(config_.scale, config_.showDebugMonitor, Centering::YesNo(isFirstResize));
		isFirstResize = false;

		// 設定ファイルでカートリッジが指定されていない場合は
		// ファイルを開くダイアログで選択する
		if (not IsValidCartridgePath(currentCartridgePath_.value_or(U"")))
		{
			currentCartridgePath_ = ChooseCartridge(config_.openCartridgeDirectory);
		}

		// ファイルが開けない場合は終了する
		if (not IsValidCartridgePath(currentCartridgePath_.value_or(U"")))
		{
			DebugPrint::Writeln(U"* Cartridge not found");
			return;
		}

		// カートリッジ読み込み
		// 対応するMBCがカートリッジをロードする
		// 対応するMBCがない場合は終了する
		if (not mem_->loadCartridge(*currentCartridgePath_))
		{
			DebugPrint::Writeln(U"* Cannot open cartridge");
			return;
		}

		// SRAMをロード
		mem_->loadSRAM();

		// BootROM
		bool enableBootROM = not config_.bootROMPath.isEmpty() && FileSystem::Exists(config_.bootROMPath);
		if (enableBootROM)
		{
			mem_->enableBootROM(config_.bootROMPath);
		}

		// [DEBUG]
		DebugPrint::Writeln(U"* Cartridge loaded:");
		DebugPrint::Writeln(U"FileName={}"_fmt(FileSystem::FileName(*currentCartridgePath_)));
		mem_->dumpCartridgeInfo();

		// CGBモードの適用
		if (mem_->isSupportedCGBMode() && config_.detectCGB)
		{
			mem_->setCGBMode(true);
			ppu_->setCGBMode(true);
			apu_->setCGBMode(true);
			cpu_->setCGBMode(true);
		}

		cpu_->reset(enableBootROM);

		// SGBモードの適用
		if (not mem_->isCGBMode() && mem_->isSupportedSGBMode() && config_.detectSGB)
		{
			mem_->setSGBMode(true);
			ppu_->setSGBMode(true);
			cpu_->setSGBMode(true);
		}

		// メモリの内容をリセット（SGB/CGBモード確定後にリセットする必要がある）
		mem_->reset();

		mainLoop_();

		// アプリケーション終了時にSRAMを保存する
		mem_->saveSRAM();
	}

	void DmgeApp::setCartridgePath(const String& cartridgePath)
	{
		currentCartridgePath_ = cartridgePath;
	}

	String DmgeApp::currentCartridgePath() const
	{
		return currentCartridgePath_.value_or(U"");
	}

	DmgeAppMode DmgeApp::mode() const
	{
		return mode_;
	}

	MooneyeTestResult DmgeApp::mooneyeTestResult() const
	{
		return mooneyeTestResult_;
	}

	void DmgeApp::mainLoop_()
	{
		FPSKeeper fpsKeeper{};

		// モニタのリフレッシュレートが 60Hz の場合は FPS 制御を Siv3D に任せる
		// そうでない場合は FPSKeeper で 60[FPS] を保つ
		if (System::GetCurrentMonitor().refreshRate == 60)
		{
			fpsKeeper.setEnable(false);
		}

		while (not quitApp_)
		{
			// メニュー表示中は専用のループへ
			if (menuOverlay_.isVisible())
			{
				apu_->pause();
				menuLoop_();
			}

			// ブレークポイントに達したらトレースモードに切り替える

			if (reachedBreakpoint_())
			{
				mode_ = DmgeAppMode::Trace;

				apu_->pause();

				DebugPrint::Writeln(U"Break: pc={:04X}"_fmt(cpu_->currentPC()));
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
					mem_->dump(addr, addr + 15);
				}

				cpu_->dump();

				// トレースモードの間は、トレースループ～メニューループ間を繰り返し遷移する
				do
				{
					traceLoop_();
					menuLoop_();
				} while (mode_ == DmgeAppMode::Trace);
			}
			else
			{
				if (enableTraceDump_)
				{
					cpu_->dump();
				}
			}

			// テストモードの場合、テスト結果をチェック

			if (config_.testMode)
			{
				mooneyeTestResult_ = cpu_->mooneyeTestResult();
				if (mooneyeTestResult_ != MooneyeTestResult::Running)
				{
					return;
				}
			}

			// CPUコマンドを1回実行する
			cpu_->run();

			tickUnits_(cpu_->consumedCycles());

			// 割り込み
			if (cpu_->interrupt())
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

				// キーボード入力処理
				// ※デバッグモニタでテキスト入力中はキーボード入力を受け付けない
				if (not processingDebugMonitorTextInput_())
				{
					commonInput_();

					// トレースモードに移行 (Ctrl+P)
					if (KeyP.down() && KeyControl.pressed())
					{
						pause_();
					}
				}

				// PPUのレンダリング結果を画面表示
				ppu_->draw(Vec2{ 0, 0 }, config_.scale);

				// APU
				if (enableAPU_ && mode_ != DmgeAppMode::Trace)
				{
					apu_->playIfBufferEnough(2000);
					apu_->pauseIfBufferNotEnough(512);
				}

				// デバッグ用モニタ表示
				if (config_.showDebugMonitor)
				{
					updateDebugMonitor_();
				}

				// デバッグモニタのテキストボックス入力中はJOYPADを更新しない
				joypad_->setEnable(not processingDebugMonitorTextInput_());

				if (config_.showFPS)
				{
					DrawStatusText(U"FPS:{:3d}"_fmt(Profiler::FPS()));
				}

				fpsKeeper.sleep();

				cyclesFromPreviousDraw_ = 0;
			}
		}
	}

	void DmgeApp::traceLoop_()
	{
		while (mode_ == DmgeAppMode::Trace)
		{
			if (not System::Update())
			{
				quitApp_ = true;
				break;
			}

			// メニューの表示状態
			const bool visibleMenu = menuOverlay_.isVisible();

			// キーボード入力処理
			// ※デバッグモニタでテキスト入力中はキーボード入力を受け付けない
			if (not processingDebugMonitorTextInput_())
			{
				commonInput_();

				// メニューを表示したタイミングでループから抜けてメニューループへ移る
				if (not visibleMenu && menuOverlay_.isVisible())
				{
					break;
				}

				// ステップ実行 (F7)
				if (KeyF7.down())
				{
					break;
				}

				// トレースモード終了 (Ctrl+P or F5)
				if ((KeyP.down() && KeyControl.pressed()) || KeyF5.down())
				{
					resume_();
					break;
				}
			}

			ppu_->draw(Point{ 0, 0 }, config_.scale);

			// デバッグ用モニタ表示
			if (config_.showDebugMonitor)
			{
				updateDebugMonitor_();
			}

			DrawStatusText(U"TRACE");
		}
	}

	void DmgeApp::menuLoop_()
	{
		while (menuOverlay_.isVisible())
		{
			if (not System::Update())
			{
				menuOverlay_.hide();
				quitApp_ = true;
				break;
			}

			ppu_->draw(Point{ 0, 0 }, config_.scale);

			// デバッグ用モニタ表示
			if (config_.showDebugMonitor)
			{
				updateDebugMonitor_();
			}

			menuOverlay_.update();
			menuOverlay_.draw();
		}
	}

	void DmgeApp::commonInput_()
	{
		// デバッグモニタ表示切り替え (F10)
		if (KeyF10.down())
		{
			toggleDebugMonitor_(true);
		}

		// [DEBUG]常にダンプ (Ctrl+D)
		if (KeyD.down() && KeyControl.pressed())
		{
			enableTraceDump_ = not enableTraceDump_;

			DebugPrint::Writeln(U"EnableTraceDump={}"_fmt(enableTraceDump_));
		}

		// パレット切り替え (Ctrl+L)
		if (KeyL.down() && KeyControl.pressed())
		{
			changePalettePreset_(1);
		}

		// リセット (Ctrl+R)
		if (KeyR.down() && KeyControl.pressed())
		{
			reset_();
		}

		// 開く (Ctrl+O)
		if (KeyO.down() && KeyControl.pressed())
		{
			openCartridge_();
		}

		// APUの各チャンネルをミュート (Key 1-4)

		if (Key1.down())
		{
			toggleAudioChannelMute_(0);
		}

		if (Key2.down())
		{
			toggleAudioChannelMute_(1);
		}

		if (Key3.down())
		{
			toggleAudioChannelMute_(2);
		}

		if (Key4.down())
		{
			toggleAudioChannelMute_(3);
		}

		// APUの使用を切替 (Key 5)
		if (Key5.down())
		{
			toggleAudio_();
		}

		// Toggle LPF (Key 6)
		if (Key6.down())
		{
			toggleAudioLPF_();
		}

		// メニューを開く (ESC or 右クリック)
		if (KeyEscape.up() || MouseR.up())
		{
			menuOverlay_.show();
		}
	}

	// メモリ書き込み時フック
	void DmgeApp::onMemoryWrite_(uint16 addr, uint8 value)
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

				apu_->pause();

				DebugPrint::Writeln(U"Break(Memory Write): pc={:04X} mem={:04X} val={:02X}"_fmt(cpu_->currentPC(), addr, value));
				cpu_->dump();
				break;
			}
		}
	}

	// 画面表示用パレットを設定する
	void DmgeApp::setPPUPalette_(int paletteIndex)
	{
		const auto& palette = paletteIndex == 0 ? config_.paletteColors : Colors::PalettePresets[paletteIndex];

		ppu_->setPaletteColors(palette);

		for (int i = 0; i < 4; ++i)
		{
			lcd_->setSGBPaletteColors(i, palette);
		}
	}

	// ブレークポイントが有効かつブレークポイントに達したか
	bool DmgeApp::reachedBreakpoint_() const
	{
		if (not config_.enableBreakpoint) return false;

		for (const auto bp : config_.breakpoints)
		{
			if (cpu_->currentPC() == bp)
			{
				return true;
			}
		}

		if (config_.breakOnLDBB && mem_->read(cpu_->currentPC()) == 0x40)
		{
			return true;
		}

		return false;
	}

	bool DmgeApp::reachedTraceDumpAddress_() const
	{
		if (not config_.traceDumpStartAddress) return false;

		for (const auto addr : config_.traceDumpStartAddress)
		{
			if (cpu_->currentPC() == addr)
			{
				return true;
			}
		}

		return false;
	}

	void DmgeApp::tickUnits_(int cycles)
	{
		int doubleSpeedFactor = mem_->isDoubleSpeed() ? 2 : 1;

		// RTC, DMA
		mem_->update(cycles);

		// タイマーを更新

		for (int i : step(cycles))
		{
			timer_->update();
		}

		// Serial

		for (int i : step(cycles))
		{
			serial_->update();
		}

		// PPU

		for (int i : step(cycles / doubleSpeedFactor))
		{
			ppu_->run();
		}

		// APU

		if (enableAPU_)
		{
			for (int i : step(cycles / doubleSpeedFactor))
			{
				bufferedSamples_ += apu_->run();
			}
		}
	}

	bool DmgeApp::checkShouldDraw_()
	{
		bool shouldDraw = false;

		// (1) オーディオバッファに1フレーム分のサンプルを書き込んだら描画する

		int doubleSpeedFactor = mem_->isDoubleSpeed() ? 2 : 1;

		if (enableAPU_)
		{
			if (bufferedSamples_ > 1.0 * sampleRate_ / 60.0 * doubleSpeedFactor)
			{
				bufferedSamples_ -= sampleRate_ / 60.0 * doubleSpeedFactor;

				shouldDraw = true;
			}
		}

		// (2) 描画されないまま一定のサイクル数が経過した場合に強制的に描画する

		cyclesFromPreviousDraw_ += cpu_->consumedCycles();

		if (cyclesFromPreviousDraw_ >= ClockFrequency / 59.50 * doubleSpeedFactor)
		{
			shouldDraw = true;
		}

		return shouldDraw;
	}

	void DmgeApp::updateDebugMonitor_()
	{
		debugMonitor_->update();

		debugMonitor_->draw(Point{ 160 * config_.scale, 0 });
	}

	void DmgeApp::initMenu_()
	{
		rootMenu_.items.push_back({
			.text = GUI::MenuItemText{
				.label = U"Reset",
				.shortcut = U"Ctrl+R",
			},
			.handler = [&]() {
				reset_();
				menuOverlay_.hide();
			}
		});

		rootMenu_.items.push_back({
			.text = GUI::MenuItemText{
				.label = U"Open cartridge...",
				.shortcut = U"Ctrl+O",
			},
			.handler = [&]() {
				openCartridge_();
				menuOverlay_.hide();
			}
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = mode_ == DmgeAppMode::Trace ? U"Resume" : U"Pause",
				};
			},
			.handler = [&]() {
				if (mode_ == DmgeAppMode::Trace)
					resume_();
				else
					pause_();
				menuOverlay_.hide();
			}
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Display scaling",
					.state = U"{} ({}x{})"_fmt(config_.scale, 160 * config_.scale, 144 * config_.scale),
				};
			},
			.handler = [&]() {
				config_.scale = Max(1, (config_.scale + 1) % (AppConfig::ScalingMax + 1));
			},
			.handlerLR = [&](bool leftPressed) {
				if (leftPressed)
				{
					config_.scale = ((config_.scale - 1 - 1 + AppConfig::ScalingMax) % AppConfig::ScalingMax) + 1;
				}
				else
				{
					config_.scale = Max(1, (config_.scale + 1) % (AppConfig::ScalingMax + 1));
				}
			}
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Palette (for DMG/SGB)",
					.state = U"{}/{}"_fmt(config_.palettePreset + 1, Colors::PalettePresetsCount)
				};
			},
			.handler = [&]() {
				changePalettePreset_(1);
			},
			.handlerLR = [&](bool leftPressed) {
				if (leftPressed)
				{
					changePalettePreset_(-1);
				}
				else
				{
					changePalettePreset_(1);
				}
			},
			.isPaletteSetting = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Controller settings...",
				};
			},
			.handler = [&]() {
				menuOverlay_.set(inputMenu_);
			},
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio",
					.state = enableAPU_ ? U"On" : U"Off",
					.shortcut = U"5",
				};
			},
			.handler = [&]() {
				toggleAudio_();
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio channel 1",
					.state = apu_->getMute(0) ? U"Off" : U"On",
					.shortcut = U"1",
				};
			},
			.handler = [&]() {
				toggleAudioChannelMute_(0);
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio channel 2",
					.state = apu_->getMute(1) ? U"Off" : U"On",
					.shortcut = U"2",
				};
			},
			.handler = [&]() {
				toggleAudioChannelMute_(1);
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio channel 3",
					.state = apu_->getMute(2) ? U"Off" : U"On",
					.shortcut = U"3",
				};
			},
			.handler = [&]() {
				toggleAudioChannelMute_(2);
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio channel 4",
					.state = apu_->getMute(3) ? U"Off" : U"On",
					.shortcut = U"4",
				};
			},
			.handler = [&]() {
				toggleAudioChannelMute_(3);
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Audio LPF",
					.state = apu_->getEnableLPF() ? U"On" : U"Off",
					.shortcut = U"6",
				};
			},
			.handler = [&]() {
				toggleAudioLPF_();
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Debug monitor",
					.state = config_.showDebugMonitor ? U"Show" : U"Hide",
					.shortcut = U"F10",
				};
			},
			.handler = [&]() {
				toggleDebugMonitor_(false);
			},
			.enableLR = true
		});

		rootMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Show FPS",
					.state = config_.showFPS ? U"Show" : U"Hide",
				};
			},
			.handler = [&]() {
				toggleShowFPS_();
			},
			.enableLR = true
		});


		const auto menuItemBorder = GUI::MenuItem{
			.text = GUI::MenuItemText{.label = U"--------" },
		};

		const auto menuItemEmpty = GUI::MenuItem{};

		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);
		rootMenu_.items.push_back(menuItemBorder);

		rootMenu_.items.push_back({
			.text = GUI::MenuItemText{.label = U"Back" },
			.handler = [&]() {
				menuOverlay_.hide();
			}
		});


		// Input settings

		inputMenu_.items.push_back({
			.text = GUI::MenuItemText{.label = U"---- Keyboard Mapping ----"},
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Right",
					.state = keyMap_->get(JoypadButtons::Right).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Right);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Right, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Left",
					.state = keyMap_->get(JoypadButtons::Left).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Left);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Left, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Up",
					.state = keyMap_->get(JoypadButtons::Up).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Up);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Up, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Down",
					.state = keyMap_->get(JoypadButtons::Down).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Down);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Down, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"A",
					.state = keyMap_->get(JoypadButtons::A).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::A);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::A, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"B",
					.state = keyMap_->get(JoypadButtons::B).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::B);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::B, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Select",
					.state = keyMap_->get(JoypadButtons::Select).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Select);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Select, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Start",
					.state = keyMap_->get(JoypadButtons::Start).name()
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Keyboard, JoypadButtons::Start);
				if (input.code())
				{
					keyMap_->set(JoypadButtons::Start, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back(menuItemEmpty);

		inputMenu_.items.push_back({
			.text = GUI::MenuItemText{.label = U"---- Gamepad Mapping ----"},
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"A",
					.state = Format(U"Button "_sv, gamepadMap_->get(JoypadButtons::A).code()),
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Gamepad, JoypadButtons::A);
				if (input.deviceType() == InputDeviceType::Gamepad)
				{
					gamepadMap_->set(JoypadButtons::A, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"B",
					.state = Format(U"Button "_sv, gamepadMap_->get(JoypadButtons::B).code()),
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Gamepad, JoypadButtons::B);
				if (input.deviceType() == InputDeviceType::Gamepad)
				{
					gamepadMap_->set(JoypadButtons::B, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Select",
					.state = Format(U"Button "_sv, gamepadMap_->get(JoypadButtons::Select).code()),
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Gamepad, JoypadButtons::Select);
				if (input.deviceType() == InputDeviceType::Gamepad)
				{
					gamepadMap_->set(JoypadButtons::Select, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back({
			.textFunc = [&]() {
				return GUI::MenuItemText{
					.label = U"Start",
					.state = Format(U"Button "_sv, gamepadMap_->get(JoypadButtons::Start).code()),
				};
			},
			.handler = [&]() {
				const auto input = InputMappingOverlay::Get(InputDeviceType::Gamepad, JoypadButtons::Start);
				if (input.deviceType() == InputDeviceType::Gamepad)
				{
					gamepadMap_->set(JoypadButtons::Start, input);
					applyInputMapping_();
				}
			}
		});

		inputMenu_.items.push_back(menuItemEmpty);

		inputMenu_.items.push_back({
			.text = GUI::MenuItemText{.label = U"Back" },
			.handler = [&]() {
				menuOverlay_.backToPrevious();
			}
		});

		menuOverlay_.set(rootMenu_);
	}

	void DmgeApp::reset_()
	{
		mode_ = DmgeAppMode::Reset;
		quitApp_ = true;
	}

	void DmgeApp::pause_()
	{
		mode_ = DmgeAppMode::Trace;

		apu_->pause();
	}

	void DmgeApp::resume_()
	{
		mode_ = DmgeAppMode::Default;
	}

	void DmgeApp::openCartridge_()
	{
		if (const auto openPath = ChooseCartridge(FileSystem::ParentPath(currentCartridgePath_.value_or(U"")));
			openPath)
		{
			currentCartridgePath_ = openPath;
			mode_ = DmgeAppMode::Reset;
			quitApp_ = true;
		}
	}

	void DmgeApp::toggleAudioChannelMute_(int channel)
	{
		apu_->setMute(channel, not apu_->getMute(channel));
	}

	void DmgeApp::toggleAudio_()
	{
		enableAPU_ = not enableAPU_;
		if (not enableAPU_)
		{
			apu_->pause();
		}
	}

	void DmgeApp::toggleAudioLPF_()
	{
		apu_->setEnableLPF(not apu_->getEnableLPF());
	}

	void DmgeApp::toggleDebugMonitor_(bool applyWindowSize)
	{
		config_.showDebugMonitor = not config_.showDebugMonitor;

		if (applyWindowSize)
		{
			SetScaleWindowSize(config_.scale, config_.showDebugMonitor);
		}
	}

	void DmgeApp::toggleShowFPS_()
	{
		config_.showFPS = not config_.showFPS;
	}

	void DmgeApp::changePalettePreset_(int changeIndex)
	{
		config_.palettePreset = (config_.palettePreset + Colors::PalettePresetsCount + changeIndex) % Colors::PalettePresetsCount;
		setPPUPalette_(config_.palettePreset);
	}

	void DmgeApp::applyInputMapping_()
	{
		joypad_->setMapping(*keyMap_, *gamepadMap_);

		config_.keyMapping = keyMap_->get();
		config_.gamepadMapping = gamepadMap_->get();
	}

	bool DmgeApp::processingDebugMonitorTextInput_() const
	{
		return config_.showDebugMonitor && debugMonitor_->isVisibleTextbox();
	}
}
