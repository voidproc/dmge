#pragma once

namespace dmge
{
	class DebugPrint
	{
	public:
		static void EnableConsole();

		template <class ...Args>
		static void Writeln(const Args& ... args)
		{
			if (enableConsole)
			{
				Console.writeln(args...);
			}
		}

	private:
		inline static bool enableConsole = false;
	};
}
