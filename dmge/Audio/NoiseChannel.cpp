#include "../stdafx.h"
#include "NoiseChannel.h"
#include "../Address.h"

namespace dmge
{
	uint8 NoiseChannel::readRegister(uint16 addr) const
	{
		if (addr == Address::NR42)
		{
			return envelope_.get();
		}
		else if (addr == Address::NR43)
		{
			return (divisorShift_ << 4) | (counterWidth_ << 3) | divisor_;
		}
		else if (addr == Address::NR44)
		{
			return (uint8)length_.getEnable() << 6;
		}

		return 0;
	}

	void NoiseChannel::step()
	{
		if (--freqTimer_ <= 0)
		{
			freqTimer_ = (divisor_ > 0 ? (divisor_ * 16) : 8) << divisorShift_;

			uint16 xorResult = (lfsr_ & 0b01) ^ ((lfsr_ & 0b10) >> 1);
			lfsr_ = (lfsr_ >> 1) | (xorResult << 14);
			if (counterWidth_ == 1)
			{
				lfsr_ &= ~(1 << 6);
				lfsr_ |= xorResult << 6;
			}
		}
	}

	void NoiseChannel::trigger()
	{
		if (getDACEnable())
			setEnable(true);

		envelope_.trigger();

		length_.trigger(64);

		lfsr_ = 0xffff;
	}

	int NoiseChannel::amplitude() const
	{
		auto amp4 = ~lfsr_ & 0x01;
		if (envelope_.initialVolume() == 0) amp4 = 0;
		return amp4 * envelope_.volume();
	}

	void NoiseChannel::stepEnvelope()
	{
		envelope_.step();
	}

	void NoiseChannel::setEnvelope(uint8 NRx2)
	{
		envelope_.set(NRx2);
	}

	void NoiseChannel::stepLength()
	{
		length_.step();
	}

	void NoiseChannel::setEnableLength(bool enable)
	{
		length_.setEnable(enable);
	}

	void NoiseChannel::setLengthTimer(uint8 NRx1)
	{
		length_.setLengthTimer(64 - (NRx1 & 0b111111));
	}

	void NoiseChannel::setExtraLengthClockCondition(bool value)
	{
		length_.setExtraLengthClockCond(value);
	}

	void NoiseChannel::setRandomness(uint8 NRx3)
	{
		divisorShift_ = NRx3 >> 4;
		counterWidth_ = (NRx3 >> 3) & 1;
		divisor_ = NRx3 & 0b111;
	}
}
