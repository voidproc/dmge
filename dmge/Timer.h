#pragma once

namespace dmge
{
	class Memory;
	class Interrupt;

	class Timer
	{
	public:
		Timer(Memory* mem, Interrupt* interrupt);

		// IOレジスタへの書き込み
		void writeRegister(uint16 addr, uint8 value);

		// IOレジスタからの読み込み
		uint8 readRegister(uint16 addr) const;

		void update();

		void resetDIV();

		uint8 div() const;

	private:
		Memory* mem_;
		Interrupt* interrupt_;

		uint8 tima_ = 0;
		uint8 tma_ = 0;
		uint8 tac_ = 0;

		uint16 divInternal_ = 0;
		uint8 divBitPrev_ = 0;
		int tmaCount_ = 0;
		int reloadingCount_ = 0;

		void abortInterrupt_();
		bool isReloading_() const;
	};
}
