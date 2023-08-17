#include "stdafx.h"
#include "AppConfig.h"
#include "DebugPrint.h"

namespace dmge
{
	namespace
	{
		Array<uint16> MakeArrayFromCommaSeparatedString(const String& str)
		{
			return str.split(U',')
				.map([](const String& s) { return ParseInt<uint16>(s, 16); });
		}
	}

	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config;

		INI ini{ ConfigFilePath };

		// Cartridge

		config.cartridgePath = ini.getOr<String>(U"Cartridge", U"");
		config.openCartridgeDirectory = ini.getOr<String>(U"OpenCartridgeDirectory", U"");
		config.detectCGB = ini.getOr<int>(U"DetectCGB", true);
		config.detectSGB = ini.getOr<int>(U"DetectSGB", true);
		config.bootROMPath = ini.getOr<String>(U"BootROM", U"");

		// Display

		config.scale = ini.getOr<int>(U"Scale", 3);
		config.showFPS = ini.getOr<int>(U"ShowFPS", false);
		config.palettePreset = ini.getOr<int>(U"PalettePreset", 0);
		config.paletteColors[0] = ColorF{ U"#" + ini.getOr<String>(U"PaletteColor0", U"e8e8e8") };
		config.paletteColors[1] = ColorF{ U"#" + ini.getOr<String>(U"PaletteColor1", U"a0a0a0") };
		config.paletteColors[2] = ColorF{ U"#" + ini.getOr<String>(U"PaletteColor2", U"585858") };
		config.paletteColors[3] = ColorF{ U"#" + ini.getOr<String>(U"PaletteColor3", U"101010") };
		config.cgbColorGamma = ini.getOr<double>(U"CGBColorGamma", 1.0);

		// DebugMonitor

		config.showDebugMonitor = ini.getOr<int>(U"ShowDebugMonitor", false);

		// Input

		config.keyMapping[0] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardRight", KeyRight.code()) };
		config.keyMapping[1] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardLeft", KeyLeft.code()) };
		config.keyMapping[2] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardUp", KeyUp.code()) };
		config.keyMapping[3] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardDown", KeyDown.code()) };
		config.keyMapping[4] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardA", KeyX.code()) };
		config.keyMapping[5] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardB", KeyZ.code()) };
		config.keyMapping[6] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardSelect", KeyBackspace.code()) };
		config.keyMapping[7] = Input{ InputDeviceType::Keyboard, ini.getOr<uint8>(U"KeyboardStart", KeyEnter.code()) };

		config.gamepadMapping[4] = Input{ InputDeviceType::Gamepad, ini.getOr<uint8>(U"GamepadButtonA", 0) };
		config.gamepadMapping[5] = Input{ InputDeviceType::Gamepad, ini.getOr<uint8>(U"GamepadButtonB", 1) };
		config.gamepadMapping[6] = Input{ InputDeviceType::Gamepad, ini.getOr<uint8>(U"GamepadButtonSelect", 2) };
		config.gamepadMapping[7] = Input{ InputDeviceType::Gamepad, ini.getOr<uint8>(U"GamepadButtonStart", 3) };

		// Audio

		config.enableAudioLPF = ini.getOr<int>(U"EnableAudioLPF", false);
		config.audioLPFConstant = ini.getOr<double>(U"AudioLPFConstant", 0.8);

		// Debug

		config.showConsole = ini.getOr<int>(U"ShowConsole", true);
		config.breakpoints = MakeArrayFromCommaSeparatedString(ini.getOr<String>(U"Breakpoint", U""));
		config.memoryWriteBreakpoints = MakeArrayFromCommaSeparatedString(ini.getOr<String>(U"MemoryWriteBreakpoint", U""));
		config.breakOnLDBB = ini.getOr<int>(U"BreakOnLDBB", false);
		config.enableBreakpoint = ini.getOr<int>(U"EnableBreakpoint", 0);
		config.dumpAddress = MakeArrayFromCommaSeparatedString(ini.getOr<String>(U"DumpAddress", U""));
		config.traceDumpStartAddress = MakeArrayFromCommaSeparatedString(ini.getOr<String>(U"TraceDumpStartAddress", U""));
		config.logFilePath = ini.getOr<String>(U"LogFilePath", U"");
		config.testMode = ini.getOr<int>(U"TestMode", false);

		return config;
	}

	namespace
	{
		String KeyValueString(const auto& key, const auto& val)
		{
			return U"{} = {}"_fmt(key, val);
		}

		String CommaSeparatedHexString(const Array<uint16>& arr)
		{
			return arr.map([](const auto& val) { return ToHex(val); }).join(U","_sv, U""_sv, U""_sv);
		}

		String CategoryComment(StringView category)
		{
			return U"\n; --------------------------------\n; {}\n; --------------------------------\n"_fmt(category);
		}
	}

	void AppConfig::save()
	{
		TextWriter writer{ ConfigFilePath };

		writer.writeln(CategoryComment(U"Cartridge"));
		writer.writeln(KeyValueString(U"Cartridge", this->cartridgePath));
		writer.writeln(KeyValueString(U"OpenCartridgeDirectory", this->openCartridgeDirectory));
		writer.writeln(KeyValueString(U"DetectCGB", (int)this->detectCGB));
		writer.writeln(KeyValueString(U"DetectSGB", (int)this->detectSGB));
		writer.writeln(KeyValueString(U"BootROM", this->bootROMPath));

		writer.writeln(CategoryComment(U"Display"));
		writer.writeln(KeyValueString(U"Scale", this->scale));
		writer.writeln(KeyValueString(U"ShowFPS", (int)this->showFPS));
		writer.writeln(KeyValueString(U"PalettePreset", this->palettePreset));
		writer.writeln(KeyValueString(U"PaletteColor0", this->paletteColors[0].toColor().toHex()));
		writer.writeln(KeyValueString(U"PaletteColor1", this->paletteColors[1].toColor().toHex()));
		writer.writeln(KeyValueString(U"PaletteColor2", this->paletteColors[2].toColor().toHex()));
		writer.writeln(KeyValueString(U"PaletteColor3", this->paletteColors[3].toColor().toHex()));
		writer.writeln(KeyValueString(U"CGBColorGamma", U"{:.2f}"_fmt(this->cgbColorGamma)));

		writer.writeln(CategoryComment(U"DebugMonitor"));
		writer.writeln(KeyValueString(U"ShowDebugMonitor", (int)this->showDebugMonitor));

		writer.writeln(CategoryComment(U"Input"));
		writer.writeln(KeyValueString(U"KeyboardRight", this->keyMapping[0].code()));
		writer.writeln(KeyValueString(U"KeyboardLeft", this->keyMapping[1].code()));
		writer.writeln(KeyValueString(U"KeyboardUp", this->keyMapping[2].code()));
		writer.writeln(KeyValueString(U"KeyboardDown", this->keyMapping[3].code()));
		writer.writeln(KeyValueString(U"KeyboardA", this->keyMapping[4].code()));
		writer.writeln(KeyValueString(U"KeyboardB", this->keyMapping[5].code()));
		writer.writeln(KeyValueString(U"KeyboardSelect", this->keyMapping[6].code()));
		writer.writeln(KeyValueString(U"KeyboardStart", this->keyMapping[7].code()));
		writer.writeln(KeyValueString(U"GamepadButtonA", this->gamepadMapping[4].code()));
		writer.writeln(KeyValueString(U"GamepadButtonB", this->gamepadMapping[5].code()));
		writer.writeln(KeyValueString(U"GamepadButtonSelect", this->gamepadMapping[6].code()));
		writer.writeln(KeyValueString(U"GamepadButtonStart", this->gamepadMapping[7].code()));

		writer.writeln(CategoryComment(U"Audio"));
		writer.writeln(KeyValueString(U"EnableAudioLPF", (int)this->enableAudioLPF));
		writer.writeln(KeyValueString(U"AudioLPFConstant", U"{:.2f}"_fmt(this->audioLPFConstant)));

		writer.writeln(CategoryComment(U"Debug"));
		writer.writeln(KeyValueString(U"ShowConsole", (int)this->showConsole));
		writer.writeln(KeyValueString(U"Breakpoint", CommaSeparatedHexString(this->breakpoints)));
		writer.writeln(KeyValueString(U"MemoryWriteBreakpoint", CommaSeparatedHexString(this->memoryWriteBreakpoints)));
		writer.writeln(KeyValueString(U"BreakOnLDBB", (int)this->breakOnLDBB));
		writer.writeln(KeyValueString(U"EnableBreakpoint", (int)this->enableBreakpoint));
		writer.writeln(KeyValueString(U"DumpAddress", CommaSeparatedHexString(this->dumpAddress)));
		writer.writeln(KeyValueString(U"TraceDumpStartAddress", CommaSeparatedHexString(this->traceDumpStartAddress)));
		writer.writeln(KeyValueString(U"LogFilePath", this->logFilePath));
		writer.writeln(KeyValueString(U"TestMode", (int)this->testMode));

		writer.close();
	}

	void AppConfig::print()
	{
		DebugPrint::Writeln(U"* AppConfig:");

		DebugPrint::Writeln(U"Cartridge={}"_fmt(cartridgePath));
		DebugPrint::Writeln(U"OpenCartridgeDirectory={}"_fmt(openCartridgeDirectory));
		DebugPrint::Writeln(U"DetectCGB={}"_fmt(detectCGB));
		DebugPrint::Writeln(U"DetectSGB={}"_fmt(detectSGB));
		DebugPrint::Writeln(U"BootROM={}"_fmt(bootROMPath));

		DebugPrint::Writeln(U"Scale={}"_fmt(scale));
		DebugPrint::Writeln(U"ShowFPS={}"_fmt(showFPS));
		DebugPrint::Writeln(U"PalettePreset={}"_fmt(palettePreset));
		DebugPrint::Writeln(U"CGBColorGamma={}"_fmt(cgbColorGamma));

		DebugPrint::Writeln(U"KeyboardRight={} ({})"_fmt(keyMapping[0].code(), keyMapping[0].name()));
		DebugPrint::Writeln(U"KeyboardLeft={} ({})"_fmt(keyMapping[1].code(), keyMapping[1].name()));
		DebugPrint::Writeln(U"KeyboardUp={} ({})"_fmt(keyMapping[2].code(), keyMapping[2].name()));
		DebugPrint::Writeln(U"KeyboardDown={} ({})"_fmt(keyMapping[3].code(), keyMapping[3].name()));
		DebugPrint::Writeln(U"KeyboardA={} ({})"_fmt(keyMapping[4].code(), keyMapping[4].name()));
		DebugPrint::Writeln(U"KeyboardB={} ({})"_fmt(keyMapping[5].code(), keyMapping[5].name()));
		DebugPrint::Writeln(U"KeyboardSelect={} ({})"_fmt(keyMapping[6].code(), keyMapping[6].name()));
		DebugPrint::Writeln(U"KeyboardStart={} ({})"_fmt(keyMapping[7].code(), keyMapping[7].name()));
		DebugPrint::Writeln(U"GamepadButtonA={}"_fmt(gamepadMapping[4].code()));
		DebugPrint::Writeln(U"GamepadButtonB={}"_fmt(gamepadMapping[5].code()));
		DebugPrint::Writeln(U"GamepadButtonSelect={}"_fmt(gamepadMapping[6].code()));
		DebugPrint::Writeln(U"GamepadButtonStart={}"_fmt(gamepadMapping[7].code()));

		DebugPrint::Writeln(U"EnableAudioLPF={}"_fmt(enableAudioLPF));
		DebugPrint::Writeln(U"AudioLPFConstant={}"_fmt(audioLPFConstant));

		DebugPrint::Writeln(U"ShowConsole={}"_fmt(showConsole));
		DebugPrint::Writeln(U"Breakpoint={}"_fmt(CommaSeparatedHexString(breakpoints)));
		DebugPrint::Writeln(U"MemoryWriteBreakpoint={}"_fmt(CommaSeparatedHexString(memoryWriteBreakpoints)));
		DebugPrint::Writeln(U"BreakOnLDBB={}"_fmt(breakOnLDBB));
		DebugPrint::Writeln(U"EnableBreakpoint={}"_fmt(enableBreakpoint));
		DebugPrint::Writeln(U"DumpAddress={}"_fmt(CommaSeparatedHexString(dumpAddress)));
		DebugPrint::Writeln(U"TraceDumpStartAddress={}"_fmt(CommaSeparatedHexString(traceDumpStartAddress)));
		DebugPrint::Writeln(U"LogFilePath={}"_fmt(logFilePath));
		DebugPrint::Writeln(U"ShowDebugMonitor={}"_fmt(showDebugMonitor));
	}
}
