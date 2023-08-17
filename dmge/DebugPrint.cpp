#include "stdafx.h"
#include "DebugPrint.h"

namespace dmge
{
	namespace
	{
		bool enableConsole = false;
		bool enableFileOutput = false;
		TextWriter textWriter{};
	}

	void DebugPrint::EnableConsole()
	{
		enableConsole = true;
	}

	void DebugPrint::EnableFileOutput(FilePathView logfilePath)
	{
		enableFileOutput = true;
		textWriter.open(logfilePath);
	}

	void DebugPrint::Writeln(const String& text)
	{
		if (enableFileOutput)
		{
			textWriter.writeln(text);
		}

		if (enableConsole)
		{
			Console.writeln(text);
		}
	}
}
