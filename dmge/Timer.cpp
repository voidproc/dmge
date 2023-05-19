#include "Timer.h"
#include "Memory.h"
#include "Address.h"
#include "BitMask/TAC.h"
#include "BitMask/InterruptFlag.h"

namespace dmge
{
	Timer::Timer(Memory* mem)
		: mem_{ mem }
	{
	}

	void Timer::update()
	{
		// DIV

		divInternal_ += 1;
		mem_->writeDirect(Address::DIV, divInternal_ >> 8);

		// TAC

		const uint8 tac = mem_->read(Address::TAC);

		uint8 timerEnable = (tac & BitMask::TAC::TimerEnable) ? 1 : 0;
		uint8 divBit = 0;

		switch (tac & BitMask::TAC::Clock)
		{
		case 0: divBit = (divInternal_ >> 9) & 1; break;
		case 1: divBit = (divInternal_ >> 3) & 1; break;
		case 2: divBit = (divInternal_ >> 5) & 1; break;
		case 3: divBit = (divInternal_ >> 7) & 1; break;
		}

		divBit = timerEnable & divBit;

		if (tmaCount_ == 0 && divBit == 0 && divBitPrev_ == 1)
		{
			// Inc TIMA
			const uint8 tima = mem_->read(Address::TIMA);
			mem_->writeDirect(Address::TIMA, tima + 1);

			// TIMA Overflows
			if (tima == 0xff)
			{
				tmaCount_ = 4;
			}
		}

		divBitPrev_ = divBit;

		if (tmaCount_ > 0)
		{
			if (--tmaCount_ == 0)
			{
				const uint8 tma = mem_->read(Address::TMA);
				mem_->writeDirect(Address::TIMA, tma);

				// Timer割り込みを要求
				const uint8 intFlag = mem_->read(Address::IF);
				mem_->write(Address::IF, intFlag | BitMask::InterruptFlag::Timer);
			}
		}
	}

	void Timer::resetInternalDIV()
	{
		divInternal_ = 0;
	}

	void Timer::abortInterrupt()
	{
		tmaCount_ = 0;
	}
}
