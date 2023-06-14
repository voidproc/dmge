#include "Channel.h"
#include "Memory.h"

namespace dmge
{
	void GetChannel1Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR10 = mem->read(Address::NR10);
		const uint8 NR11 = mem->read(Address::NR11);
		const uint8 NR12 = mem->read(Address::NR12);
		//const uint8 NR13 = mem->read(Address::NR13);
		//const uint8 NR14 = mem->read(Address::NR14);

		ch.sweepPeriod = (NR10 >> 4) & 0b111;
		ch.sweepDir = (NR10 >> 3) & 1;
		ch.sweepShift = NR10 & 0b111;
		ch.duty = NR11 >> 6;
		ch.envVol = NR12 >> 4;
		ch.envDir = (NR12 >> 3) & 1;
		ch.envPeriod = NR12 & 0b111;
	}

	void GetChannel2Data(Memory* mem, ChannelData& ch)
	{
		const uint8 NR21 = mem->read(Address::NR21);
		const uint8 NR22 = mem->read(Address::NR22);
		//const uint8 NR23 = mem->read(Address::NR23);
		//const uint8 NR24 = mem->read(Address::NR24);

		ch.duty = NR21 >> 6;
		ch.envVol = NR22 >> 4;
		ch.envDir = (NR22 >> 3) & 1;
		ch.envPeriod = NR22 & 0b111;
	}

	void GetChannel3Data(Memory* mem, ChannelData& ch)
	{
		//const uint8 NR30 = mem->read(Address::NR30);
		//const uint8 NR31 = mem->read(Address::NR31);
		const uint8 NR32 = mem->read(Address::NR32);
		//const uint8 NR33 = mem->read(Address::NR33);
		//const uint8 NR34 = mem->read(Address::NR34);

		ch.outputLevel = (NR32 >> 5) & 0b11;
	}

	void GetChannel4Data(Memory* mem, ChannelData& ch)
	{
		//const uint8 NR41 = mem->read(Address::NR41);
		const uint8 NR42 = mem->read(Address::NR42);
		const uint8 NR43 = mem->read(Address::NR43);
		//const uint8 NR44 = mem->read(Address::NR44);

		ch.envVol = NR42 >> 4;
		ch.envDir = (NR42 >> 3) & 1;
		ch.envPeriod = NR42 & 0b111;
		ch.divisorShift = NR43 >> 4;
		ch.counterWidth = (NR43 >> 3) & 1;
		ch.divisor = NR43 & 0b111;
	}

	int SquareWaveAmplitude(int duty, int dutyPos)
	{
		constexpr std::array<uint8, 4> waveDutyTable = {
			0b00000001, 0b00000011, 0b00001111, 0b11111100,
		};

		return (waveDutyTable[duty] >> (7 - dutyPos)) & 1;
	}

	int FrequencyReplacedLower(int originalFreq, uint8 lower)
	{
		return (originalFreq & 0x700) | lower;
	}

	int FrequencyReplacedHigher(int originalFreq, uint8 higher)
	{
		return ((higher & 0x7) << 8) | (originalFreq & 0xff);
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

	void Channel::trigger()
	{
		setEnable(true);

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
			periodTimer_ = data_.envPeriod;

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

	void Channel::stepEnvelope()
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

	void Channel::stepSweep()
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

	void Channel::stepLength()
	{
		if (enableLength_)
		{
			//if (ch_ == Channels::Ch1) Console.writeln(U"{:10d} Channel::doLength() enableLength_=true lengthTimer_={}"_fmt(g_clock, lengthTimer_));

			if (lengthTimer_ > 0)
			{
				--lengthTimer_;
				//if (ch_ == Channels::Ch1) Console.writeln(U"{:10d} Channel1 doLength() clocked lengthTimer_={}"_fmt(g_clock, lengthTimer_));
				//if (ch_ == Channels::Ch2) Console.writeln(U"{:10d} Channel_ doLength() clocked lengthTimer_={}"_fmt(g_clock, lengthTimer_));

				//if (ch_ == Channels::Ch1 && lengthTimer_ == 0) Console.writeln(U"{:10d} Channel1 doLength() lengthTimer reached to 0"_fmt(g_clock));
				//if (ch_ == Channels::Ch2 && lengthTimer_ == 0) Console.writeln(U"{:10d} Channel_ doLength() lengthTimer reached to 0"_fmt(g_clock));
			}

			if (lengthTimer_ == 0)
			{
				setEnable(false);
				//if (ch_ == Channels::Ch1) Console.writeln(U"{:10d} Channel1 doLength() expired"_fmt(g_clock));
				//if (ch_ == Channels::Ch2) Console.writeln(U"{:10d} Channel_ doLength() expired"_fmt(g_clock));
			}
		}
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

	bool Channel::getEnable() const
	{
		return enabled_;
	}

	void Channel::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	void Channel::setEnableLength(bool enable)
	{
		{
			// フレームシーケンサの次のステップが長さカウンターをクロックしないものである場合、
			// NRx4への書き込み時に余分な長さクロックが発生します。
			// この場合、長さカウンタが以前は無効で、現在は有効で、長さカウンタがゼロでない場合、
			// 長さカウンタはデクリメントされます。
			// Refer: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior

			if (not enableLength_ && enable && extraLengthClockCond_)
			{
				enableLength_ = enable;
				stepLength();
				return;
			}
		}

		enableLength_ = enable;
		//if (ch_==Channels::Ch1) Console.writeln(U"{:10d} Channel1 setEnableLength({})"_fmt(g_clock, enable));
		//if (ch_==Channels::Ch2) Console.writeln(U"{:10d} Channel_ setEnableLength({})"_fmt(g_clock, enable));
	}

	void Channel::setLengthTimer(uint8 reg)
	{
		switch (ch_)
		{
		case Channels::Ch1:
		case Channels::Ch2:
		case Channels::Ch4:
			lengthTimer_ = 64 - (reg & 0b111111);
			//if (ch_ == Channels::Ch1) Console.writeln(U"{:10d} Channel1 setLengthTimer({})"_fmt(g_clock, lengthTimer_));
			//if (ch_ == Channels::Ch2) Console.writeln(U"{:10d} Channel_ setLengthTimer({})"_fmt(g_clock, lengthTimer_));
			break;

		case Channels::Ch3:
			lengthTimer_ = 256 - reg;
			break;
		}
	}

	void Channel::setExtraLengthClockCondition(bool value)
	{
		extraLengthClockCond_ = value;
	}

	void Channel::setFrequencyLow(uint8 freqLow)
	{
		freq_ = (freq_ & 0x700) | freqLow;
	}

	void Channel::setFrequencyHigh(uint8 freqHigh)
	{
		freq_ = ((freqHigh & 0x7) << 8) | (freq_ & 0xff);
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


	// VolumeEnvelope

	void VolumeEnvelope::step()
	{
		if (envPeriod)
		{
			if (periodTimer_ > 0) periodTimer_--;

			if (periodTimer_ == 0)
			{
				periodTimer_ = envPeriod;

				if ((currentVolume_ < 0xf && envDir == 1) || (currentVolume_ > 0 && envDir == 0))
				{
					currentVolume_ += (envDir == 1) ? 1 : -1;
				}
			}
		}
	}

	void VolumeEnvelope::trigger()
	{
		currentVolume_ = envVol;
		periodTimer_ = envPeriod;
	}

	void VolumeEnvelope::set(uint8 NRx2)
	{
		envVol = NRx2 >> 4;
		envDir = (NRx2 >> 3) & 1;
		envPeriod = NRx2 & 0b111;
	}

	int VolumeEnvelope::volume() const
	{
		return currentVolume_;
	}

	int VolumeEnvelope::initialVolume() const
	{
		return envVol;
	}


	// FrequencySweep

	FrequencySweep::FrequencySweep(ChannelBase* channel)
		: channel_{ channel }
	{
	}

	int FrequencySweep::step(int freq)
	{
		if (sweepTimer_ > 0) sweepTimer_--;

		if (sweepTimer_ == 0)
		{
			sweepTimer_ = (sweepPeriod > 0) ? sweepPeriod : 8;

			if (sweepEnabled_ && sweepPeriod > 0)
			{
				const int newFreq = calcSweepFrequency_();

				if (newFreq <= 2047 && sweepShift > 0)
				{
					freq = shadowFreq_ = newFreq;

					// Check sweepEnabled
					calcSweepFrequency_();
				}
			}
		}

		return freq;
	}

	void FrequencySweep::trigger(int freq)
	{
		shadowFreq_ = freq;
		sweepTimer_ = (sweepPeriod > 0) ? sweepPeriod : 8;
		sweepEnabled_ = sweepPeriod != 0 || sweepShift != 0;
		if (sweepShift != 0) calcSweepFrequency_();
	}

	void FrequencySweep::set(uint8 NRx0)
	{
		sweepPeriod = (NRx0 >> 4) & 0b111;
		sweepDir = (NRx0 >> 3) & 1;
		sweepShift = NRx0 & 0b111;
	}

	int FrequencySweep::calcSweepFrequency_()
	{
		int newFreq = shadowFreq_ >> sweepShift;

		if (sweepDir == 0)
			newFreq = shadowFreq_ + newFreq;
		else
			newFreq = shadowFreq_ - newFreq;

		if (newFreq > 2047) channel_->setEnable(false);
		return newFreq;
	}


	// LengthCounter

	LengthCounter::LengthCounter(ChannelBase* channel)
		: channel_{ channel }
	{
	}

	void LengthCounter::step()
	{
		if (enableLength_)
		{
			if (lengthTimer_ > 0)
			{
				--lengthTimer_;
			}

			if (lengthTimer_ == 0)
			{
				channel_->setEnable(false);
			}
		}
	}

	void LengthCounter::trigger(int newLengthTimer)
	{
		if (lengthTimer_ == 0)
		{
			lengthTimer_ = newLengthTimer;
		}
	}

	void LengthCounter::setEnable(bool enable)
	{
		// フレームシーケンサの次のステップが長さカウンターをクロックしないものである場合、
		// NRx4への書き込み時に余分な長さクロックが発生します。
		// この場合、長さカウンタが以前は無効で、現在は有効で、長さカウンタがゼロでない場合、
		// 長さカウンタはデクリメントされます。
		// Refer: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior

		if (not enableLength_ && enable && extraLengthClockCond_)
		{
			enableLength_ = enable;
			step();
			return;
		}

		enableLength_ = enable;
	}

	bool LengthCounter::getEnable() const
	{
		return enableLength_;
	}

	void LengthCounter::setLengthTimer(int value)
	{
		lengthTimer_ = value;
	}

	void LengthCounter::setExtraLengthClockCond(bool cond)
	{
		extraLengthClockCond_ = cond;
	}


	// ChannelBase

	bool ChannelBase::getEnable() const
	{
		return enabled_;
	}

	void ChannelBase::setEnable(bool enable)
	{
		enabled_ = enable;
	}


	// SquareChannel

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


	// WaveChannel

	WaveChannel::WaveChannel(Memory* mem)
		: mem_{ mem }
	{
	}

	void WaveChannel::step()
	{
		if (--freqTimer_ <= 0)
		{
			freqTimer_ = (2048 - freq_) * 2;
			waveRAMOffset_ = (waveRAMOffset_ + 1) % 32;
		}
	}

	void WaveChannel::trigger()
	{
		setEnable(true);

		length_.trigger(256);

		waveRAMOffset_ = 0;
	}

	int WaveChannel::amplitude() const
	{
		uint8 wave = mem_->read(Address::WaveRAM + waveRAMOffset_ / 2);
		uint8 amp3 = (wave >> ((1 - (waveRAMOffset_ % 2)) * 4)) & 0xf;
		constexpr int shift[4] = { 4, 0, 1, 2 };
		return amp3 >> shift[waveOutputLevel_];
	}

	void WaveChannel::stepLength()
	{
		length_.step();
	}

	void WaveChannel::setEnableLength(bool enable)
	{
		length_.setEnable(enable);
	}

	void WaveChannel::setLengthTimer(uint8 NRx1)
	{
		length_.setLengthTimer(256 - NRx1);
	}

	void WaveChannel::setExtraLengthClockCondition(bool value)
	{
		length_.setExtraLengthClockCond(value);
	}

	void WaveChannel::setFrequencyLow(uint8 freqLow)
	{
		freq_ = FrequencyReplacedLower(freq_, freqLow);
	}

	void WaveChannel::setFrequencyHigh(uint8 freqHigh)
	{
		freq_ = FrequencyReplacedHigher(freq_, freqHigh);
	}

	int WaveChannel::getFrequency()
	{
		return freq_;
	}

	void WaveChannel::setWaveOutputLevel(uint8 NRx2)
	{
		waveOutputLevel_ = (NRx2 >> 5) & 0b11;
	}


	// NoiseChannel

	void NoiseChannel::step()
	{
		if (--freqTimer_ <= 0)
		{
			freqTimer_ = (divisor > 0 ? (divisor * 16) : 8) << divisorShift;

			uint16 xorResult = (lfsr_ & 0b01) ^ ((lfsr_ & 0b10) >> 1);
			lfsr_ = (lfsr_ >> 1) | (xorResult << 14);
			if (counterWidth == 1)
			{
				lfsr_ &= ~(1 << 6);
				lfsr_ |= xorResult << 6;
			}
		}
	}

	void NoiseChannel::trigger()
	{
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
		divisorShift = NRx3 >> 4;
		counterWidth = (NRx3 >> 3) & 1;
		divisor = NRx3 & 0b111;
	}

}
