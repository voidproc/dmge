﻿#include "APU.h"
#include "Memory.h"
#include "Address.h"

namespace dmge
{
	ChannelData GetChannel1Data(Memory* mem)
	{
		const uint8 NR10 = mem->read(Address::NR10);
		const uint8 NR11 = mem->read(Address::NR11);
		const uint8 NR12 = mem->read(Address::NR12);
		const uint8 NR13 = mem->read(Address::NR13);
		const uint8 NR14 = mem->read(Address::NR14);

		return ChannelData{
			.sweepPeriod = (NR10 >> 4) & 0b111,
			.sweepDir = (NR10 >> 3) & 1,
			.sweepShift = NR10 & 0b111,
			.duty = NR11 >> 6,
			.lengthTimer = NR11 & 63,
			.envVol = NR12 >> 4,
			.envDir = (NR12 >> 3) & 1,
			.envPeriod = NR12 & 0b111,
			.trigger = (NR14 >> 7) == 1,
			.enableLength = ((NR14 >> 6) & 1) == 1,
			.freq = NR13 | ((NR14 & 0b111) << 8),
		};
	}

	ChannelData GetChannel2Data(Memory* mem)
	{
		const uint8 NR21 = mem->read(Address::NR21);
		const uint8 NR22 = mem->read(Address::NR22);
		const uint8 NR23 = mem->read(Address::NR23);
		const uint8 NR24 = mem->read(Address::NR24);

		return ChannelData{
			.duty = NR21 >> 6,
			.lengthTimer = NR21 & 63,
			.envVol = NR22 >> 4,
			.envDir = (NR22 >> 3) & 1,
			.envPeriod = NR22 & 0b111,
			.trigger = (NR24 >> 7) == 1,
			.enableLength = ((NR24 >> 6) & 1) == 1,
			.freq = NR23 | ((NR24 & 0b111) << 8),
		};
	}

	ChannelData GetChannel3Data(Memory* mem)
	{
		const uint8 NR30 = mem->read(Address::NR30);
		const uint8 NR31 = mem->read(Address::NR31);
		const uint8 NR32 = mem->read(Address::NR32);
		const uint8 NR33 = mem->read(Address::NR33);
		const uint8 NR34 = mem->read(Address::NR34);

		return ChannelData{
			.enable = (NR30 >> 7) == 1,
			.outputLevel = (NR32 >> 5) & 0b11,
			.lengthTimer = NR31,
			.trigger = (NR34 >> 7) == 1,
			.enableLength = ((NR34 >> 6) & 1) == 1,
			.freq = NR33 | ((NR34 & 0b111) << 8),
		};
	}

	ChannelData GetChannel4Data(Memory* mem)
	{
		const uint8 NR41 = mem->read(Address::NR41);
		const uint8 NR42 = mem->read(Address::NR42);
		const uint8 NR43 = mem->read(Address::NR43);
		const uint8 NR44 = mem->read(Address::NR44);

		return ChannelData{
			.lengthTimer = NR41 & 0b111111,
			.envVol = NR42 >> 4,
			.envDir = (NR42 >> 3) & 1,
			.envPeriod = NR42 & 0b111,
			.trigger = (NR44 >> 7) == 1,
			.enableLength = ((NR44 >> 6) & 1) == 1,
			.divisorShift = NR43 >> 4,
			.counterWidth = (NR43 >> 3) & 1,
			.divisor = NR43 & 0b111,
		};
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
			data_ = GetChannel1Data(mem_);
			break;

		case Channels::Ch2:
			data_ = GetChannel2Data(mem_);
			break;

		case Channels::Ch3:
			data_ = GetChannel3Data(mem_);
			break;

		case Channels::Ch4:
			data_ = GetChannel4Data(mem_);
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
				freqTimer_ = (2048 - data_.freq) * 2;
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

				//bool isShort = data_.counterWidth == 1;
				//if (lfsr_ == 0) lfsr_ = 1;
				//lfsr_ += lfsr_ + (((lfsr_ >> (isShort ? 6 : 14)) ^ (lfsr_ >> (isShort ? 5 : 13))) & 1);

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
			freq_ = data_.freq;
			currentVolume_ = data_.envVol;
			periodTimer_ = data_.envPeriod;

			shadowFreq_ = freq_;
			sweepTimer_ = (data_.sweepPeriod > 0) ? data_.sweepPeriod : 8;
			sweepEnabled_ = data_.sweepPeriod != 0 || data_.sweepShift != 0;
			if (data_.sweepShift != 0) calcSweepFrequency_();

			if (lengthTimer_ == 0)
				lengthTimer_ = 64 - data_.lengthTimer;
			//Console.writeln(U"freq:{} envVol:{} envPeriod:{} lengthTimer:{}"_fmt(freq_, data_.envVol, data_.envPeriod, lengthTimer_));
			break;

		case Channels::Ch2:
			dutyPos_ = 0;
			freq_ = data_.freq;
			currentVolume_ = data_.envVol;

			if (lengthTimer_ == 0)
				lengthTimer_ = 64 - data_.lengthTimer;
			break;

		case Channels::Ch3:
			freq_ = data_.freq;

			if (lengthTimer_ == 0)
				lengthTimer_ = 256 - data_.lengthTimer;

			waveRAMOffset_ = 0;
			break;

		case Channels::Ch4:
			currentVolume_ = data_.envVol;
			periodTimer_ = data_.envPeriod;

			if (lengthTimer_ == 0)
				lengthTimer_ = 64 - data_.lengthTimer;

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
		return trigger_ && data_.trigger && getDACEnable();
	}

	int Channel::amplitude() const
	{
		switch (ch_)
		{
		case Channels::Ch1:
		case Channels::Ch2:
			return SquareWaveAmplitude(data_.duty, dutyPos_) * currentVolume_;// *chEnabledByLength_;

		case Channels::Ch3:
		{
			if (not data_.enable) return 0;

			uint8 wave = mem_->read(Address::WaveRAM + waveRAMOffset_ / 2);
			uint8 amp3 = (wave >> ((1 - (waveRAMOffset_ % 2)) * 4)) & 0xf;
			const int shift[4] = { 4, 0, 1, 2 };
			amp3 = (amp3 >> shift[data_.outputLevel]);// *chEnabledByLength_;
			return amp3;
		}

		case Channels::Ch4:
		{
			auto amp4 = ~lfsr_ & 0x01;
			if (data_.envVol == 0) amp4 = 0;
			return amp4 * currentVolume_;// *chEnabledByLength_;
		}

		default:
			break;
		}

		return 0;
	}

	void Channel::checkDAC()
	{
		if (not getDACEnable())
		{
			setEnable(false);
		}
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
	}

	bool Channel::getEnable() const
	{
		const uint8 NR52 = mem_->read(Address::NR52);

		switch (ch_)
		{
		case Channels::Ch1: return ((NR52 >> 0) & 1) == 1;
		case Channels::Ch2: return ((NR52 >> 1) & 1) == 1;
		case Channels::Ch3: return ((NR52 >> 2) & 1) == 1;
		case Channels::Ch4: return ((NR52 >> 3) & 1) == 1;
		}

		return false;
	}

	void Channel::setEnable(bool enable)
	{
		uint8 NR52 = mem_->read(Address::NR52);

		int shift = 0;

		switch (ch_)
		{
		case Channels::Ch1: shift = 0; break;
		case Channels::Ch2: shift = 1; break;
		case Channels::Ch3: shift = 2; break;
		case Channels::Ch4: shift = 3; break;
		}

		NR52 &= ~(1 << shift);
		NR52 |= (enable ? 1 : 0) << shift;
		mem_->write(Address::NR52, NR52);
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

	int Channel::calcSweepFrequency_()
	{
		int newFreq = shadowFreq_ >> data_.sweepShift;
		newFreq = shadowFreq_ + (data_.sweepDir == 0) ? -newFreq : newFreq;
		if (newFreq > 2047) setEnable(false);
		return newFreq;
	}


	APU::APU(Memory* mem)
		:
		mem_{ mem },
		ch1_{ mem, Channels::Ch1 },
		ch2_{ mem, Channels::Ch2 },
		ch3_{ mem, Channels::Ch3 },
		ch4_{ mem, Channels::Ch4 }
	{
		wave_.resize(44100);
	}

	void APU::update()
	{
		ch1_.fetch();
		ch2_.fetch();
		ch3_.fetch();
		ch4_.fetch();

		ch1_.updateFrequencyTimer();
		ch2_.updateFrequencyTimer();
		ch3_.updateFrequencyTimer();
		ch4_.updateFrequencyTimer();

		// Frame Sequencer (FS)

		const uint8 div = mem_->read(Address::DIV);

		// FS clocks

		bool onVolumeClock = false;
		bool onSweepClock = false;
		bool onLengthClock = false;

		if ((prevDiv_ & 0b100000) && (div & 0b100000) == 0)
		{
			fsClock_++;

			if ((fsClock_ % 8) == 7) onVolumeClock = true;
			if (((fsClock_ - 2) % 4) == 0) onSweepClock = true;
			if ((fsClock_ % 2) == 0) onLengthClock = true;
		}

		// トリガー
		// エンベロープ、スイープ、Length制御の初期化

		if (ch1_.onTrigger())
			ch1_.doTrigger();

		if (ch2_.onTrigger())
			ch2_.doTrigger();

		if (ch3_.onTrigger())
			ch3_.doTrigger();

		if (ch4_.onTrigger())
			ch4_.doTrigger();

		// スイープ

		if (onSweepClock)
		{
			ch1_.doSweep();
		}

		// Length

		if (onLengthClock)
		{
			ch1_.doLength();
			ch2_.doLength();
			ch3_.doLength();
			ch4_.doLength();
		}

		// エンベロープ

		if (onVolumeClock)
		{
			ch1_.doEnvelope();
			ch2_.doEnvelope();
			ch4_.doEnvelope();
		}

		// もしDACがoffならチャンネルをoffにする
		//ch1_.checkDAC();
		//ch2_.checkDAC();
		//ch3_.checkDAC();
		//ch4_.checkDAC();

		// Save previous state
		prevDiv_ = div;

		// Output audio
		// (CPUFreq / SampleRate) ==> 4194304 / 44100 ==> Every 95.1 T-cycles

		if (cycles_ == 0)
		{
			const uint8 NR52 = mem_->read(Address::NR52);
			const uint8 masterSwitch = NR52 >> 7;
			const uint8 ch1Enabled = (NR52 >> 0) & 1;
			const uint8 ch2Enabled = (NR52 >> 1) & 1;
			const uint8 ch3Enabled = (NR52 >> 2) & 1;
			const uint8 ch4Enabled = (NR52 >> 3) & 1;

			const auto dacInput1 = ch1_.amplitude() * masterSwitch * ch1Enabled;
			const auto dacInput2 = ch2_.amplitude() * masterSwitch * ch2Enabled;
			const auto dacInput3 = ch3_.amplitude() * masterSwitch * ch3Enabled;
			const auto dacInput4 = ch4_.amplitude() * masterSwitch * ch4Enabled;

			const auto dacOutput1 = (dacInput1 / 7.5) - 1.0;  // -1.0 ～ +1.0
			const auto dacOutput2 = (dacInput2 / 7.5) - 1.0;  // -1.0 ～ +1.0
			const auto dacOutput3 = (dacInput3 / 7.5) - 1.0;  // -1.0 ～ +1.0
			const auto dacOutput4 = (dacInput4 / 7.5) - 1.0;  // -1.0 ～ +1.0

			double sample = (dacOutput1 + dacOutput2 + dacOutput3 + dacOutput4) / 4.0;
			//double sample = (dacOutput1);

			wave_[samples_++].set(sample, sample);

			if (samples_ >= wave_.size())
			{
				audio_ = Audio{ wave_ };
				audio_.play();
				samples_ = 0;
			}
		}

		cycles_ = (cycles_ + 1) % 95;
	}

	void APU::setFrequency(Channels ch, int freq)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setFrequency(freq); return;
		case Channels::Ch2: ch2_.setFrequency(freq); return;
		case Channels::Ch3: ch3_.setFrequency(freq); return;
		case Channels::Ch4: ch4_.setFrequency(freq); return;
		}
	}

	void APU::trigger(Channels ch)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setTriggerFlag(); return;
		case Channels::Ch2: ch2_.setTriggerFlag(); return;
		case Channels::Ch3: ch3_.setTriggerFlag(); return;
		case Channels::Ch4: ch4_.setTriggerFlag(); return;
		}
	}

	void APU::setLengthTimer(Channels ch, uint8 reg)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setLengthTimer(reg); return;
		case Channels::Ch2: ch2_.setLengthTimer(reg); return;
		case Channels::Ch3: ch3_.setLengthTimer(reg); return;
		case Channels::Ch4: ch4_.setLengthTimer(reg); return;
		}
	}
}
