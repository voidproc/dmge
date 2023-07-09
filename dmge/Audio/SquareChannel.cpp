#include "SquareChannel.h"
#include "SquareWave.h"
#include "Frequency.h"
#include "../Address.h"

namespace dmge
{
	namespace
	{
		int LimitFrequency(int freq, int samplingFreq = 44100)
		{
			if (131072.0 / (2048 - freq) > samplingFreq / 2)
			{
				return 2048 - 131072.0 / (samplingFreq / 2);
			}

			return freq;
		}
	}

	uint8 SquareChannel::readRegister(uint16 addr) const
	{
		if (addr == Address::NR10)
		{
			return sweep_.get();
		}
		else if (addr == Address::NR11 || addr == Address::NR21)
		{
			return duty_ << 6;
		}
		else if (addr == Address::NR12 || addr == Address::NR22)
		{
			return envelope_.get();
		}
		else if (addr == Address::NR14 || addr == Address::NR24)
		{
			return (uint8)length_.getEnable() << 6;
		}

		return 0;
	}

	void SquareChannel::step()
	{
		if (--freqTimer_ <= 0)
		{
			// Frequency Timerをリセット
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

		// Frequency Timerをリセット
		freqTimer_ = (2048 - freq_) * 4;
	}

	int SquareChannel::amplitude() const
	{
		if (LimitFrequency(freq_) != freq_) return 0;

		return SquareWaveAmplitude(duty_, dutyPos_) * envelope_.volume();
	}

	void SquareChannel::setDuty(uint8 NRx1)
	{
		duty_ = NRx1 >> 6;
	}

	void SquareChannel::resetDutyPosition()
	{
		dutyPos_ = 0;
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

	int SquareChannel::getFrequency() const
	{
		return freq_;
	}
}
