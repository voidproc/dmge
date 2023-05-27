#include "AppConfig.h"
#include "DebugPrint.h"

namespace dmge
{
	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config;

		INI ini{ U"config.ini" };

		config.cartridgePath = ParseOr<String>(ini[U"cartridge"], U"");

		config.breakpoints = ParseOr<String>(ini[U"breakpoint"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.breakpointsMemWrite = ParseOr<String>(ini[U"breakpointmemw"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.dumpAddress = ParseOr<String>(ini[U"dumpaddress"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.enableBreakpoint = ParseOr<int>(ini[U"enablebreakpoint"], false);

		config.showFPS = ParseOr<int>(ini[U"showfps"], true);

		config.scale = ParseOr<int>(ini[U"scale"], 3);

		config.showConsole = ParseOr<int>(ini[U"showconsole"], true);

		return config;
	}

	void AppConfig::print()
	{
		DebugPrint::Writeln(U"* AppConfig:");

		DebugPrint::Writeln(U"cartridgePath={}"_fmt(cartridgePath));

		if (not breakpoints.empty())
		{
			const auto breakpointsText = breakpoints
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"breakpoints={}"_fmt(breakpointsText));
		}

		if (not breakpointsMemWrite.empty())
		{
			const auto breakpointsText = breakpointsMemWrite
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"breakpointsMemWrite={}"_fmt(breakpointsText));
		}

		if (not dumpAddress.empty())
		{
			const auto addrText = dumpAddress
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			DebugPrint::Writeln(U"dumpAddress={}"_fmt(addrText));
		}

		DebugPrint::Writeln(U"enableBreakpoint={}"_fmt(enableBreakpoint));
		DebugPrint::Writeln(U"showFPS={}"_fmt(showFPS));
	}
}
