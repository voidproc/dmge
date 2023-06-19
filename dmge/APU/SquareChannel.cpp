#include "SquareChannel.h"
#include "SquareWave.h"
#include "Frequency.h"

namespace dmge
{
	void SquareChannel::step()
	{
		if (--freqTimer_ <= 0)
		{
			freqTimer_ = (2048 - freq_) * 4;
			dutyPos_ = (dutyPos_ + 1) % 8;
		}
	}

	void SquareChannel::trigger()
	{
		if (getDACEnable())
			setEnable(true);

		sweep_.trigger(freq_);

		envelope_.trigger();

		length_.trigger(64);
	}

	int SquareChannel::amplitude() const
	{
		return SquareWaveAmplitude(duty_, dutyPos_) * envelope_.volume();
	}

	void SquareChannel::setDuty(uint8 NRx1)
	{
		duty_ = NRx1 >> 6;
	}

	void SquareChannel::stepEnvelope()
	{
		envelope_.step();
	}

	void SquareChannel::setEnvelope(uint8 NRx2)
	{
		envelope_.set(NRx2);
	}

	void SquareChannel::stepSweep()
	{
		freq_ = sweep_.step(freq_);
	}

	void SquareChannel::setSweep(uint8 NRx0)
	{
		sweep_.set(NRx0);
	}

	void SquareChannel::stepLength()
	{
		length_.step();
	}

	void SquareChannel::setEnableLength(bool enable)
	{
		length_.setEnable(enable);
	}

	void SquareChannel::setLengthTimer(uint8 NRx1)
	{
		length_.setLengthTimer(64 - (NRx1 & 0b111111));
	}

	void SquareChannel::setExtraLengthClockCondition(bool value)
	{
		length_.setExtraLengthClockCond(value);
	}

	void SquareChannel::setFrequencyLow(uint8 freqLow)
	{
		freq_ = FrequencyReplacedLower(freq_, freqLow);
	}

	void SquareChannel::setFrequencyHigh(uint8 freqHigh)
	{
		freq_ = FrequencyReplacedHigher(freq_, freqHigh);
	}

	int SquareChannel::getFrequency()
	{
		return freq_;
	}
}
