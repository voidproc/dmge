#pragma once

namespace dmge
{
	class Memory;
	class CPU;

	class DebugMonitor
	{
	public:
		DebugMonitor(Memory* mem, CPU* cpu);

		void draw(Point pos);

		bool isVisibleTextbox();

	private:
		Memory* mem_;
		CPU* cpu_;

		uint16 dumpAddress_ = 0x0000;

		bool showDumpAddressTextbox_ = false;
		TextEditState textStateDumpAddress_;
	};
}
