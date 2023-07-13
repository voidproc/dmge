#include "AppConfig.h"
#include "DebugPrint.h"

namespace dmge
{
	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config;

		INI ini{ U"config.ini" };

		config.cartridgePath = ParseOr<String>(ini[U"Cartridge"], U"");

		config.breakpoints = ParseOr<String>(ini[U"Breakpoint"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.breakpointsMemWrite = ParseOr<String>(ini[U"BreakpointMemW"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.traceDumpStartAddress = ParseOr<String>(ini[U"TraceDumpStartAddress"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.dumpAddress = ParseOr<String>(ini[U"DumpAddress"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.enableBreakpoint = ParseOr<int>(ini[U"EnableBreakpoint"], false);

		config.showFPS = ParseOr<int>(ini[U"ShowFPS"], true);

		config.scale = ParseOr<int>(ini[U"Scale"], 3);

		config.showConsole = ParseOr<int>(ini[U"ShowConsole"], true);

		config.logFilePath = ParseOr<String>(ini[U"LogFilePath"], U"");

		config.showDebugMonitor = ParseOr<int>(ini[U"ShowDebugMonitor"], true);

		config.breakOnLDBB = ParseOr<int>(ini[U"BreakOnLDBB"], false);

		config.bootROMPath = ParseOr<String>(ini[U"BootROM"], U"");

		config.gamepadButtonAssign.buttonA = ParseOr<int>(ini[U"GamepadButtonA"], 0);
		config.gamepadButtonAssign.buttonB = ParseOr<int>(ini[U"GamepadButtonB"], 1);
		config.gamepadButtonAssign.buttonSelect = ParseOr<int>(ini[U"GamepadButtonSelect"], 2);
		config.gamepadButtonAssign.buttonStart = ParseOr<int>(ini[U"GamepadButtonStart"], 3);

		return config;
	}

	void AppConfig::print()
	{
		DebugPrint::Writeln(U"* AppConfig:");

		DebugPrint::Writeln(U"Cartridge={}"_fmt(cartridgePath));

		if (not breakpoints.empty())
		{
			const auto breakpointsText = breakpoints
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"Breakpoint={}"_fmt(breakpointsText));
		}

		if (not breakpointsMemWrite.empty())
		{
			const auto breakpointsText = breakpointsMemWrite
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"BreakpointMemW={}"_fmt(breakpointsText));
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

		DebugPrint::Writeln(U"EnableBreakpoint={}"_fmt(enableBreakpoint));
		DebugPrint::Writeln(U"ShowFPS={}"_fmt(showFPS));
		DebugPrint::Writeln(U"Scale={}"_fmt(scale));
		DebugPrint::Writeln(U"ShowConsole={}"_fmt(showConsole));
		DebugPrint::Writeln(U"LogFilePath={}"_fmt(logFilePath));
		DebugPrint::Writeln(U"ShowDebugMonitor={}"_fmt(showDebugMonitor));

		DebugPrint::Writeln(U"BootROM={}"_fmt(bootROMPath));
	}
}
