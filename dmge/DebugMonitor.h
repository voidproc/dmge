#pragma once

namespace dmge
{
	class Memory;
	class CPU;
	class APU;

	class DebugMonitor
	{
	public:
		DebugMonitor(Memory* mem, CPU* cpu, APU* apu);

		void update();

		void draw(const Point& pos);

		bool isVisibleTextbox() const;

	private:
		Memory* mem_;
		CPU* cpu_;
		APU* apu_;

		uint16 dumpAddress_ = 0x0000;

		bool showDumpAddressTextbox_ = false;
		TextEditState textStateDumpAddress_;
	};
}
