#include "AppConfig.h"

namespace dmge
{
	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config{ U"", {}, {}, false };

		INI ini{ U"config.ini" };

		if (ini.hasGlobalValue(U"cartridge"))
		{
			config.cartridgePath = ini.getGlobalValue(U"cartridge");
		}

		if (ini.hasGlobalValue(U"breakpoint"))
		{
			config.breakpoints = ini.getGlobalValue(U"breakpoint")
				.split(U',')
				.map([](const String& s) { return ParseInt<uint16>(s, 16); });
		}

		if (ini.hasGlobalValue(U"breakpointmemw"))
		{
			config.breakpointsMemWrite = ini.getGlobalValue(U"breakpointmemw")
				.split(U',')
				.map([](const String& s) { return ParseInt<uint16>(s, 16); });
		}

		if (ini.hasGlobalValue(U"enablebreakpoint"))
		{
			config.enableBreakpoint = static_cast<bool>(ParseInt<int>(ini.getGlobalValue(U"enablebreakpoint")));
		}

		if (ini.hasGlobalValue(U"showfps"))
		{
			config.showFPS = static_cast<bool>(ParseInt<int>(ini.getGlobalValue(U"showfps")));
		}

		return config;
	}

	void AppConfig::print()
	{
		if (not cartridgePath.empty())
		{
			Console.writeln(U"cartridgePath={}"_fmt(cartridgePath));
		}

		if (not breakpoints.empty())
		{
			const auto breakpointsText = breakpoints
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			Console.writeln(U"breakpoints={}"_fmt(breakpointsText));
		}

		if (not breakpointsMemWrite.empty())
		{
			const auto breakpointsText = breakpointsMemWrite
				.map([](auto x) { return U"{:04X}"_fmt(x); })
				.join(U","_sv, U""_sv, U""_sv);
			Console.writeln(U"breakpointsMemWrite={}"_fmt(breakpointsText));
		}

		Console.writeln(U"enableBreakpoint={}"_fmt(enableBreakpoint));
		Console.writeln(U"showFPS={}"_fmt(showFPS));
	}
}
