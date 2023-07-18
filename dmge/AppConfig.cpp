#include "AppConfig.h"
#include "DebugPrint.h"

namespace dmge
{
	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config;

		INI ini{ U"config.ini" };

		config.cartridgePath = ini.getOr<String>(U"Cartridge", U"");

		config.openCartridgeDirectory = ini.getOr<String>(U"OpenCartridgeDirectory", U"");

		config.breakpoints = ini.getOr<String>(U"Breakpoint", U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.memoryWriteBreakpoints = ini.getOr<String>(U"MemoryWriteBreakpoint", U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.traceDumpStartAddress = ini.getOr<String>(U"TraceDumpStartAddress", U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.dumpAddress = ini.getOr<String>(U"DumpAddress", U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.enableBreakpoint = ini.getOr<int>(U"EnableBreakpoint", 0);

		config.showFPS = ini.getOr<int>(U"ShowFPS", true);

		config.scale = ini.getOr<int>(U"Scale", 3);

		config.cgbColorGamma = ini.getOr<double>(U"CGBColorGamma", 1.0);

		config.showConsole = ini.getOr<int>(U"ShowConsole", true);

		config.logFilePath = ini.getOr<String>(U"LogFilePath", U"");

		config.showDebugMonitor = ini.getOr<int>(U"ShowDebugMonitor", true);

		config.breakOnLDBB = ini.getOr<int>(U"BreakOnLDBB", false);

		config.bootROMPath = ini.getOr<String>(U"BootROM", U"");

		config.gamepadButtonAssign.buttonA = ini.getOr<int>(U"GamepadButtonA", 0);
		config.gamepadButtonAssign.buttonB = ini.getOr<int>(U"GamepadButtonB", 1);
		config.gamepadButtonAssign.buttonSelect = ini.getOr<int>(U"GamepadButtonSelect", 2);
		config.gamepadButtonAssign.buttonStart = ini.getOr<int>(U"GamepadButtonStart", 3);

		config.testMode = ini.getOr<int>(U"TestMode", false);

		return config;
	}

	void AppConfig::print()
	{
		DebugPrint::Writeln(U"* AppConfig:");

		DebugPrint::Writeln(U"Cartridge={}"_fmt(cartridgePath));

		DebugPrint::Writeln(U"OpenCartridgeDirectory={}"_fmt(openCartridgeDirectory));

		if (not breakpoints.empty())
		{
			const auto breakpointsText = breakpoints
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"Breakpoint={}"_fmt(breakpointsText));
		}

		if (not memoryWriteBreakpoints.empty())
		{
			const auto breakpointsText = memoryWriteBreakpoints
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"MemoryWriteBreakpoint={}"_fmt(breakpointsText));
		}

		if (not traceDumpStartAddress.empty())
		{
			const auto addrText = traceDumpStartAddress
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"TraceDumpStartAddress={}"_fmt(addrText));
		}

		if (not dumpAddress.empty())
		{
			const auto addrText = dumpAddress
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"DumpAddress={}"_fmt(addrText));
		}

		DebugPrint::Writeln(U"BreakOnLDBB={}"_fmt(breakOnLDBB));
		DebugPrint::Writeln(U"EnableBreakpoint={}"_fmt(enableBreakpoint));
		DebugPrint::Writeln(U"ShowFPS={}"_fmt(showFPS));
		DebugPrint::Writeln(U"Scale={}"_fmt(scale));
		DebugPrint::Writeln(U"CGBColorGamma={}"_fmt(cgbColorGamma));
		DebugPrint::Writeln(U"ShowConsole={}"_fmt(showConsole));
		DebugPrint::Writeln(U"LogFilePath={}"_fmt(logFilePath));
		DebugPrint::Writeln(U"ShowDebugMonitor={}"_fmt(showDebugMonitor));
		DebugPrint::Writeln(U"BootROM={}"_fmt(bootROMPath));
	}
}
