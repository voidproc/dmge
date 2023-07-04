#pragma once

namespace dmge
{
	class DebugPrint
	{
	public:
		static void EnableConsole();

		static void EnableFileOutput(FilePathView logfilePath);

		static void Writeln(const String& text);

		template <class ...Args>
		static void Writeln(const Args& ... args)
		{
			Writeln(Format(args...));
		}
	};
}
