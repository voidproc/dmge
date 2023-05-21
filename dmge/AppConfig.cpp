#include "AppConfig.h"

namespace dmge
{
	AppConfig AppConfig::LoadConfig()
	{
		AppConfig config{ U"", {}, {}, false };

		INI ini{ U"config.ini" };

		config.cartridgePath = ParseOr<String>(ini[U"cartridge"], U"");

		config.breakpoints = ParseOr<String>(ini[U"breakpoint"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.breakpointsMemWrite = ParseOr<String>(ini[U"breakpointmemw"], U"")
			.split(U',')
			.map([](const String& s) { return ParseInt<uint16>(s, 16); });

		config.enableBreakpoint = ParseOr<int>(ini[U"enablebreakpoint"], false);

		config.showFPS = ParseOr<int>(ini[U"showfps"], true);

		config.scale = ParseOr<int>(ini[U"scale"], 3);

		return config;
	}

	void AppConfig::print()
	{
		Console.writeln(U"cartridgePath={}"_fmt(cartridgePath));

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
