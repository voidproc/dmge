#pragma once

namespace dmge
{
	class Memory;

	class DMA
	{
	public:
		DMA(Memory* mem);

		void start(uint8 valueFF46);

		void update(int cycles);

		bool running() const;

	private:
		Memory* mem_;
		bool running_ = false;
		uint16 srcAddr_ = 0;
		int16 cycleCount_ = 0;
		uint16 offset_ = 0;
	};
}
