#pragma once

namespace dmge
{
	struct AppConfig
	{
		String cartridgePath;
		Array<uint16> breakpoints;
		Array<uint16> breakpointsMemWrite;
		bool enableBreakpoint;

		static AppConfig LoadConfig();

		void print();
	};
}
