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

	void Timer::writeRegister(uint16 addr, uint8 value)
	{
		if (addr == Address::DIV)
		{
			divInternal_ = 0;
		}
		else if (addr == Address::TIMA)
		{
			if (isReloading_()) return;

			abortInterrupt_();
			tima_ = value;
		}
		else if (addr == Address::TMA)
		{
			tma_ = value;
		}
		else if (addr == Address::TAC)
		{
			tac_ = value & 0b111;
		}
	}

	uint8 Timer::readRegister(uint16 addr) const
	{
		if (addr == Address::DIV)
		{
			return divInternal_ >> 8;
		}
		else if (addr == Address::TIMA)
		{
			return tima_;
		}
		else if (addr == Address::TMA)
		{
			return tma_;
		}
		else if (addr == Address::TAC)
		{
			return tac_ | 0xf8;
		}

		return 0;
	}

	void Timer::update()
	{
		// DIV

		divInternal_ += 1;

		// TAC

		uint8 timerEnable = (tac_ & BitMask::TAC::TimerEnable) ? 1 : 0;
		uint8 divBit = 0;

		switch (tac_ & BitMask::TAC::Clock)
		{
		case 0: divBit = (divInternal_ >> 9) & 1; break;
		case 1: divBit = (divInternal_ >> 3) & 1; break;
		case 2: divBit = (divInternal_ >> 5) & 1; break;
		case 3: divBit = (divInternal_ >> 7) & 1; break;
		}

		divBit = timerEnable & divBit;


		// DIV の選択された bit の立下り時に TIMA をインクリメントする

		if (tmaCount_ == 0 && divBit == 0 && divBitPrev_ == 1)
		{
			const uint8 tima = tima_;
			++tima_;

			// - オーバーフローする場合、0xff + 1 == 0 が書き込まれる → OK
			// - そこから 4 T-cycles の間、TIMA は TMA でなく 0 になる必要がある
			//   → tmaCount_ == 0 になるまで TMA からのリロードが行われない → OK

			// TIMA がオーバーフローしたら、TMA からのリロードまで 4 T-cycles 待つためのカウンタを設定
			if (tima == 0xff)
			{
				tmaCount_ = 4 + 1;
			}
		}

		divBitPrev_ = divBit;

		// - 待ち時間カウンタ tmaCount_ が設定されている場合、カウンタをデクリメントし、
		//   カウンタが 0 になった時点で TMA からのリロードと割り込み要求をする
		// - このカウンタが設定されている間に TIMA への書き込みがあった場合は、TMA からのリロードと割り込みが中断される必要がある
		//   → TIMA への書き込み時に abortInterrupt() が呼ばれカウンタが 0 になるので OK
		// - ただし TMA からのリロードが発生するのと ** 同じ T-cycle で ** TIMA への書き込みがあった場合は、その書き込みは無視される必要がある
		//   → …と GBEDG では記載があるが、難しいのであきらめる
		// - Pan Docs の "Timer Obscure Behaviour" の記載では M-cycle 単位で書いてあるので、
		//   リロードが発生した M-cycle のあいだ (reloadingCount_ > 0) TIMA への書き込みを無視する & TMA への書き込みを反映するようにする 

		// Overflow : -- ## -- -- -- -- -- -- -- --
		// Reload   : -- -- -- -- -- ## ## ## ## --
		// reloading: 00 00 00 00 00 04 03 02 01 00
		// T-cycles : 00 01 02 03 04 05 06 07 08 09
		// TIMA     : ff 00 00 00 00 88 88 55 55 55
		// tmaCount : 00 04 03 02 01 00 00 00 00 00
		// TMA      : 88 88 88 88 88 88 88 55 55 55

		if (tmaCount_ > 0)
		{
			if (--tmaCount_ == 0)
			{
				// オーバーフローからの待ち時間が終わったので、TMA からのリロードと割り込み要求をする

				// Timer割り込みを要求
				const uint8 intFlag = mem_->read(Address::IF);
				mem_->write(Address::IF, intFlag | BitMask::InterruptFlag::Timer);

				// リロードが発生している M-cycle が終わるまでカウントする
				reloadingCount_ = 4 + 1;
			}
		}

		// リロードが発生している M-cycle のあいだに TMA への書き込みがあった場合は TIMA へ反映する
		if (reloadingCount_ > 0)
		{
			--reloadingCount_;

			// TMA からのリロード
			tima_ = tma_;
		}
	}

	void Timer::abortInterrupt_()
	{
		tmaCount_ = 0;
	}

	bool Timer::isReloading_() const
	{
		return reloadingCount_ > 0;
	}
}
