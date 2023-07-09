#pragma once

namespace dmge
{
	class Memory;
	class CPU;
	class APU;
	class Interrupt;

	class DebugMonitor
	{
	public:
		// 表示サイズは 横60 * 縦40 [chars] くらい
		inline static constexpr Size size{ 5 * 60, 10 * 40 };

		DebugMonitor(Memory* mem, CPU* cpu, APU* apu, Interrupt* interrupt);

		~DebugMonitor();

		void update();

		void draw(const Point& pos) const;

		bool isVisibleTextbox() const;

	private:
		Memory* mem_;
		CPU* cpu_;
		APU* apu_;
		Interrupt* interrupt_;

		// メモリダンプ用

		uint16 dumpAddress_ = 0x0000;
		bool showDumpAddressTextbox_ = false;
		TextEditState textStateDumpAddress_;
	};
}
