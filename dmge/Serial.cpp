#include "stdafx.h"
#include "Serial.h"
#include "Address.h"
#include "Interrupt.h"
#include "BitMask/InterruptFlag.h"

namespace dmge
{
	namespace
	{
		int SerialClockCycles(SerialClockSpeed speed)
		{
			return speed == SerialClockSpeed::Normal ? 512 : 16;
		}
	}

	Serial::Serial(Interrupt& interrupt)
		: interrupt_{ interrupt }
	{
	}

	void Serial::writeRegister(uint16 addr, uint8 value)
	{
		switch (addr)
		{
		case Address::SB:
			transferData_ = value;
			break;

		case Address::SC:
		{
			const auto source = ToEnum<SerialClockSource>(value & 1);

			if (source != SerialClockSource::Internal) break;

			if (value >> 7)
			{
				remainBits_ = (uint8)8;
				clockSpeed_ = ToEnum<SerialClockSpeed>((value >> 1) & 1);
				clockSource_ = source;

				clock_ = SerialClockCycles(clockSpeed_);
			}
			break;
		}
		}
	}

	uint8 Serial::readRegister(uint16 addr) const
	{
		switch (addr)
		{
		case Address::SB:
			return transferData_;

		case Address::SC:
			return ((uint8)transfering() << 7) | ((FromEnum(clockSpeed_)) << 1) | (FromEnum(clockSource_));
		}

		return 0;
	}

	bool Serial::transfering() const
	{
		return remainBits_.has_value();
	}

	void Serial::update()
	{
		if (not transfering()) return;

		if (clock_ > 0)
		{
			if (--clock_ == 0)
			{
				transferData_ = (transferData_ << 1) | 1;
				*remainBits_ -= 1;

				if (*remainBits_ == 0)
				{
					// 転送が終わった
					remainBits_.reset();
					interrupt_.request(BitMask::InterruptFlagBit::Serial);
				}
				else
				{
					clock_ = SerialClockCycles(clockSpeed_);
				}
			}
		}
	}
}
