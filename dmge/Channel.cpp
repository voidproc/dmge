#include "Channel.h"
#include "Memory.h"

namespace dmge
{
	void GetChannel1Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR10 = mem->read(Address::NR10);
		const uint8 NR11 = mem->read(Address::NR11);
		const uint8 NR12 = mem->read(Address::NR12);
		const uint8 NR13 = mem->read(Address::NR13);
		const uint8 NR14 = mem->read(Address::NR14);

		ch.sweepPeriod = (NR10 >> 4) & 0b111;
		ch.sweepDir = (NR10 >> 3) & 1;
		ch.sweepShift = NR10 & 0b111;
		ch.duty = NR11 >> 6;
		ch.lengthTimer = NR11 & 63;
		ch.envVol = NR12 >> 4;
		ch.envDir = (NR12 >> 3) & 1;
		ch.envPeriod = NR12 & 0b111;
		ch.enableLength = ((NR14 >> 6) & 1) == 1;
	}

	void GetChannel2Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR21 = mem->read(Address::NR21);
		const uint8 NR22 = mem->read(Address::NR22);
		const uint8 NR23 = mem->read(Address::NR23);
		const uint8 NR24 = mem->read(Address::NR24);

		ch.duty = NR21 >> 6;
		ch.lengthTimer = NR21 & 63;
		ch.envVol = NR22 >> 4;
		ch.envDir = (NR22 >> 3) & 1;
		ch.envPeriod = NR22 & 0b111;
		ch.enableLength = ((NR24 >> 6) & 1) == 1;
	}

	void GetChannel3Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR30 = mem->read(Address::NR30);
		const uint8 NR31 = mem->read(Address::NR31);
		const uint8 NR32 = mem->read(Address::NR32);
		const uint8 NR33 = mem->read(Address::NR33);
		const uint8 NR34 = mem->read(Address::NR34);

		ch.enable = (NR30 >> 7) == 1;
		ch.outputLevel = (NR32 >> 5) & 0b11;
		ch.lengthTimer = NR31;
		ch.enableLength = ((NR34 >> 6) & 1) == 1;
	}

	void GetChannel4Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR41 = mem->read(Address::NR41);
		const uint8 NR42 = mem->read(Address::NR42);
		const uint8 NR43 = mem->read(Address::NR43);
		const uint8 NR44 = mem->read(Address::NR44);

		ch.lengthTimer = NR41 & 0b111111;
		ch.envVol = NR42 >> 4;
		ch.envDir = (NR42 >> 3) & 1;
		ch.envPeriod = NR42 & 0b111;
		ch.enableLength = ((NR44 >> 6) & 1) == 1;
		ch.divisorShift = NR43 >> 4;
		ch.counterWidth = (NR43 >> 3) & 1;
		ch.divisor = NR43 & 0b111;
	}

	int SquareWaveAmplitude(int duty, int dutyPos)
	{
		const std::array<uint8, 4> waveDutyTable = {
			0b00000001, 0b00000011, 0b00001111, 0b11111100,
		};

		return (waveDutyTable[duty] >> (7 - dutyPos)) & 1;
	}


	Channel::Channel(Memory* mem, Channels ch)
		: mem_{ mem }, ch_{ ch }
	{
	}

	void Channel::fetch()
	{
		switch (ch_)
		{
		case Channels::Ch1:
			GetChannel1Data(mem_, data_);
			break;

		case Channels::Ch2:
			GetChannel2Data(mem_, data_);
			break;

		case Channels::Ch3:
			GetChannel3Data(mem_, data_);
			break;

		case Channels::Ch4:
			GetChannel4Data(mem_, data_);
			break;

		default:
			break;
		}
	}

	void Channel::updateFrequencyTimer()
	{
		if (--freqTimer_ <= 0)
		{
			switch (ch_)
			{
			case Channels::Ch1:
			case Channels::Ch2:
				freqTimer_ = (2048 - freq_) * 4;
				dutyPos_ = (dutyPos_ + 1) % 8;
				break;

			case Channels::Ch3:
				freqTimer_ = (2048 - freq_) * 2;
				waveRAMOffset_ = (waveRAMOffset_ + 1) % 32;
				break;

			case Channels::Ch4:
			{
				freqTimer_ = (data_.divisor > 0 ? (data_.divisor * 16) : 8) << data_.divisorShift;

				uint16 xorResult = (lfsr_ & 0b01) ^ ((lfsr_ & 0b10) >> 1);
				lfsr_ = (lfsr_ >> 1) | (xorResult << 14);
				if (data_.counterWidth == 1)
				{
					lfsr_ &= ~(1 << 6);
					lfsr_ |= xorResult << 6;
				}
				break;
			}

			default:
				break;
			}

		}
	}

	void Channel::doTrigger()
	{
		setEnable(true);

		trigger_ = false;

		switch (ch_)
		{
		case Channels::Ch1:
			dutyPos_ = 0;
			currentVolume_ = data_.envVol;
			periodTimer_ = data_.envPeriod;

			shadowFreq_ = freq_;
			sweepTimer_ = (data_.sweepPeriod > 0) ? data_.sweepPeriod : 8;
			sweepEnabled_ = data_.sweepPeriod != 0 || data_.sweepShift != 0;
			if (data_.sweepShift != 0) calcSweepFrequency_();

			if (lengthTimer_ == 0)
				lengthTimer_ = 64;
			break;

		case Channels::Ch2:
			dutyPos_ = 0;
			currentVolume_ = data_.envVol;

			if (lengthTimer_ == 0)
				lengthTimer_ = 64;
			break;

		case Channels::Ch3:
			if (lengthTimer_ == 0)
				lengthTimer_ = 256;

			waveRAMOffset_ = 0;
			break;

		case Channels::Ch4:
			currentVolume_ = data_.envVol;
			periodTimer_ = data_.envPeriod;

			if (lengthTimer_ == 0)
				lengthTimer_ = 64;

			lfsr_ = 0xffff;
			break;

		default:
			break;
		}
	}

	void Channel::doEnvelope()
	{
		if (data_.envPeriod)
		{
			if (periodTimer_ > 0) periodTimer_--;

			if (periodTimer_ == 0)
			{
				periodTimer_ = data_.envPeriod;

				if ((currentVolume_ < 0xf && data_.envDir == 1) || (currentVolume_ > 0 && data_.envDir == 0))
				{
					currentVolume_ += (data_.envDir == 1) ? 1 : -1;
				}
			}
		}
	}

	void Channel::doSweep()
	{
		if (sweepTimer_ > 0) sweepTimer_--;

		if (sweepTimer_ == 0)
		{
			sweepTimer_ = (data_.sweepPeriod > 0) ? data_.sweepPeriod : 8;

			if (sweepEnabled_ && data_.sweepPeriod > 0)
			{
				const int newFreq = calcSweepFrequency_();

				if (newFreq <= 2047 && data_.sweepShift > 0)
				{
					freq_ = shadowFreq_ = newFreq;

					// Check sweepEnabled
					calcSweepFrequency_();
				}
			}
		}
	}

	void Channel::doLength()
	{
		if (data_.enableLength)
		{
			if (lengthTimer_ > 0) --lengthTimer_;

			if (lengthTimer_ == 0)
			{
				setEnable(false);
			}
		}
	}

	void Channel::setTriggerFlag()
	{
		trigger_ = true;
	}

	bool Channel::onTrigger() const
	{
		return trigger_ && getDACEnable();
	}

	int Channel::amplitude() const
	{
		switch (ch_)
		{
		case Channels::Ch1:
		case Channels::Ch2:
			return SquareWaveAmplitude(data_.duty, dutyPos_) * currentVolume_;

		case Channels::Ch3:
		{
			if (not data_.enable) return 0;

			uint8 wave = mem_->read(Address::WaveRAM + waveRAMOffset_ / 2);
			uint8 amp3 = (wave >> ((1 - (waveRAMOffset_ % 2)) * 4)) & 0xf;
			const int shift[4] = { 4, 0, 1, 2 };
			amp3 = (amp3 >> shift[data_.outputLevel]);
			return amp3;
		}

		case Channels::Ch4:
		{
			auto amp4 = ~lfsr_ & 0x01;
			if (data_.envVol == 0) amp4 = 0;
			return amp4 * currentVolume_;
		}

		default:
			break;
		}

		return 0;
	}

	bool Channel::getDACEnable() const
	{
		switch (ch_)
		{
		case Channels::Ch1:
			return (mem_->read(Address::NR12) & 0xf8) != 0;

		case Channels::Ch2:
			return (mem_->read(Address::NR22) & 0xf8) != 0;

		case Channels::Ch3:
			return data_.enable;

		case Channels::Ch4:
			return (mem_->read(Address::NR42) & 0xf8) != 0;

		default:
			break;
		}

		return false;
	}

	bool Channel::getEnable() const
	{
		return enabled_;
	}

	void Channel::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	void Channel::setLengthTimer(uint8 reg)
	{
		switch (ch_)
		{
		case Channels::Ch1:
		case Channels::Ch2:
		case Channels::Ch4:
			lengthTimer_ = 64 - (reg & 0b111111);
			break;

		case Channels::Ch3:
			lengthTimer_ = 256 - reg;
			break;
		}
	}

	void Channel::setFrequency(int freq)
	{
		freq_ = freq;
	}

	int Channel::getFrequency()
	{
		return freq_;
	}

	int Channel::calcSweepFrequency_()
	{
		int newFreq = shadowFreq_ >> data_.sweepShift;

		if (data_.sweepDir == 0)
			newFreq = shadowFreq_ + newFreq;
		else
			newFreq = shadowFreq_ - newFreq;

		if (newFreq > 2047) setEnable(false);
		return newFreq;
	}
}
