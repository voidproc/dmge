#include "APU.h"
#include "Memory.h"
#include "Address.h"
#include "Timing.h"

namespace dmge
{
	APUStream::APUStream()
	{
		// 最大1s分のバッファ
		wave_.resize(44100);
	}

	APUStream::~APUStream()
	{
	}

	void APUStream::getAudio(float* left, float* right, size_t samplesToWrite)
	{
		const int buffer = bufferRemain();
		const int writable = (buffer >= samplesToWrite) ? samplesToWrite : buffer;

		for (const auto i : step(writable))
		{
			*(left++) = wave_[posHead_].left;
			*(right++) = wave_[posHead_].right;

			posHead_ = (posHead_ + 1) % wave_.size();
		}

		for (const auto i : step(samplesToWrite - writable))
		{
			*(left++) = 0;
			*(right++) = 0;
		}
	}

	bool APUStream::hasEnded()
	{
		return false;
	}

	void APUStream::rewind()
	{
	}

	void APUStream::pushSample(float left, float right)
	{
		wave_[posTail_].left = left;
		wave_[posTail_].right = right;

		posTail_ = (posTail_ + 1) % wave_.size();
	}

	int APUStream::bufferRemain() const
	{
		if (posTail_ >= posHead_)
		{
			return posTail_ - posHead_;
		}

		return posTail_ - posHead_ + wave_.size();
	}

	int APUStream::bufferMaxSize() const
	{
		return wave_.size();
	}


	APU::APU(Memory* mem, int sampleRate)
		:
		mem_{ mem },
		sampleRate_{ sampleRate },
		ch1_{ mem, Channels::Ch1 },
		ch2_{ mem, Channels::Ch2 },
		ch3_{ mem, Channels::Ch3 },
		ch4_{ mem, Channels::Ch4 },
		apuStream_{ std::make_shared<APUStream>() },
		audio_{ apuStream_ },

		ch1__{},
		ch2__{},
		ch3__{ mem_ },
		ch4__{}
	{
	}

	int APU::run()
	{
		// バッファが十分なら書き込まない

		if (apuStream_->bufferRemain() > sampleRate_ / 8)
		{
			return 0;
		}

		// マスタースイッチがOffならAPUを停止する

		const uint8 NR52 = mem_->read(Address::NR52);
		const uint8 masterSwitch = NR52 >> 7;
		if (masterSwitch == 0)
		{
			audio_.pause();
			return 0;
		}

		ch1_.fetch();
		ch2_.fetch();
		ch3_.fetch();
		ch4_.fetch();

		ch1_.updateFrequencyTimer();
		ch2_.updateFrequencyTimer();
		ch3_.updateFrequencyTimer();
		ch4_.updateFrequencyTimer();

		ch1__.step();
		ch2__.step();
		ch3__.step();
		ch4__.step();

		// Frame Sequencer (FS)

		const uint8 div = mem_->read(Address::DIV);

		// FS clocks

		bool onVolumeClock = false;
		bool onSweepClock = false;
		bool onLengthClock = false;

		if ((prevDiv_ & 0b10000) && (div & 0b10000) == 0)
		{
			fsClock_++;

			if ((fsClock_ % 8) == 7) onVolumeClock = true;
			if ((fsClock_ & 3) == 2) onSweepClock = true;
			if ((fsClock_ % 2) == 0) onLengthClock = true;

			const bool extraLengthClockCond = ((fsClock_ + 1) % 2) != 0;
			ch1_.setExtraLengthClockCondition(extraLengthClockCond);
			ch1__.setExtraLengthClockCondition(extraLengthClockCond);
			ch2_.setExtraLengthClockCondition(extraLengthClockCond);
			ch2__.setExtraLengthClockCondition(extraLengthClockCond);
			ch3_.setExtraLengthClockCondition(extraLengthClockCond);
			ch3__.setExtraLengthClockCondition(extraLengthClockCond);
			ch4_.setExtraLengthClockCondition(extraLengthClockCond);
			ch4__.setExtraLengthClockCondition(extraLengthClockCond);

			//if (onLengthClock)
			//	Console.writeln(U"{:10d} onLengthClock"_fmt(g_clock));
		}

		// スイープ

		if (onSweepClock)
		{
			ch1_.stepSweep();
			ch1__.stepSweep();
		}

		// Length

		if (onLengthClock)
		{
			ch1_.stepLength();
			ch1__.stepLength();
			ch2_.stepLength();
			ch2__.stepLength();
			ch3_.stepLength();
			ch3__.stepLength();
			ch4_.stepLength();
			ch4__.stepLength();
		}

		// エンベロープ

		if (onVolumeClock)
		{
			ch1_.stepEnvelope();
			ch1__.stepEnvelope();
			ch2_.stepEnvelope();
			ch2__.stepEnvelope();
			ch4_.stepEnvelope();
			ch4__.stepEnvelope();
		}

		// Save previous state
		prevDiv_ = div;

		// Output audio
		// (CPUFreq / SampleRate) ==> 4194304 / 44100 ==> Every 95.1 T-cycles

		cycles_ += 1.0;

		if (cycles_ >= 1.0 * ClockFrequency / sampleRate_)
		{
			cycles_ -= 1.0 * ClockFrequency / sampleRate_;

			const std::array<int, 4> chAmp = {
				1 * ch1__.amplitude() * ch1__.getEnable(),
				1 * ch2__.amplitude() * ch2__.getEnable(),
				1 * ch3__.amplitude() * ch3__.getEnable(),
				1 * ch4__.amplitude() * ch4__.getEnable(),
			};

			// Input / Panning

			std::array<int, 4> leftInput = { 0, 0, 0, 0 };
			std::array<int, 4> rightInput = { 0, 0, 0, 0 };

			const uint8 NR51 = mem_->read(Address::NR51);

			for (int i : step(4))
			{
				if ((NR51 >> i) & 1) rightInput[i] = chAmp[i];
				if ((NR51 >> (i + 4)) & 1) leftInput[i] = chAmp[i];
			}

			// DAC Output

			double left = 0;
			double right = 0;

			for (int i : step(4))
			{
				left += (leftInput[i] / 7.5) - 1.0;
				right += (rightInput[i] / 7.5) - 1.0;
			}

			// Master Volume

			const uint8 NR50 = mem_->read(Address::NR50);

			const double leftVolume = (((NR50 >> 4) & 0b111) + 1) / 8.0;
			const double rightVolume = (((NR50 >> 0) & 0b111) + 1) / 8.0;

			apuStream_->pushSample(left * leftVolume / 4.0, right * rightVolume / 4.0);

			return 1;
		}

		return 0;
	}

	void APU::playIfBufferEnough(int thresholdSamples)
	{
		if (audio_.isPlaying()) return;
		if (apuStream_->bufferRemain() < thresholdSamples) return;

		audio_.play();
	}

	void APU::pauseIfBufferNotEnough(int thresholdSamples)
	{
		if (not audio_.isPlaying()) return;
		if (apuStream_->bufferRemain() > thresholdSamples) return;

		audio_.pause();
	}

	void APU::pause()
	{
		audio_.pause();
	}

	void APU::writeRegister(uint16 addr, uint8 value)
	{
		// NR52

		if (addr == Address::NR52)
		{
			ch1_.setEnable(((value >> 0) & 1) == 1);
			ch2_.setEnable(((value >> 1) & 1) == 1);
			ch3_.setEnable(((value >> 2) & 1) == 1);
			ch4_.setEnable(((value >> 3) & 1) == 1);

			ch1__.setEnable(((value >> 0) & 1) == 1);
			ch2__.setEnable(((value >> 1) & 1) == 1);
			ch3__.setEnable(((value >> 2) & 1) == 1);
			ch4__.setEnable(((value >> 3) & 1) == 1);


			// APUがoffになったとき、APUレジスタが全てクリアされる
			if ((value & 0x80) == 0)
			{
				for (uint16 apuReg = Address::NR10; apuReg <= Address::NR51; apuReg++)
				{
					mem_->write(apuReg, 0);
				}
			}
		}
		else if ((mem_->read(Address::NR52) & 0x80) == 0)
		{
			// Ignore if APU is off
			return;
		}

		// Sweep

		else if (addr == Address::NR10)
		{
			ch1__.setSweep(value);
		}

		// APU - Reload Length Timer

		else if (addr == Address::NR11)
		{
			ch1_.setLengthTimer(value);
			ch1__.setLengthTimer(value);
			ch1__.setDuty(value);
		}
		else if (addr == Address::NR21)
		{
			ch2_.setLengthTimer(value);
			ch2__.setLengthTimer(value);
			ch2__.setDuty(value);
		}
		else if (addr == Address::NR31)
		{
			ch3_.setLengthTimer(value);
			ch3__.setLengthTimer(value);
		}
		else if (addr == Address::NR41)
		{
			ch4_.setLengthTimer(value);
			ch4__.setLengthTimer(value);
		}

		// APU - Enable DAC / Envelope

		else if (addr == Address::NR12)
		{
			ch1__.setEnvelope(value);

			if ((value & 0xf8) == 0)
			{
				ch1_.setEnable(false);
				ch1__.setEnable(false);
			}
		}
		else if (addr == Address::NR22)
		{
			ch2__.setEnvelope(value);

			if ((value & 0xf8) == 0)
			{
				ch2_.setEnable(false);
				ch2__.setEnable(false);
			}
		}
		else if (addr == Address::NR30)
		{
			if ((value >> 7) == 0)
			{
				ch3_.setEnable(false);
				ch3__.setEnable(false);
			}
		}
		else if (addr == Address::NR42)
		{
			ch4__.setEnvelope(value);

			if ((value & 0xf8) == 0)
			{
				ch4_.setEnable(false);
				ch4__.setEnable(false);
			}
		}

		// Wave channel volume

		else if (addr == Address::NR32)
		{
			ch3__.setWaveOutputLevel(value);
		}

		// APU - Update Frequency

		else if (addr == Address::NR13)
		{
			ch1_.setFrequencyLow(value);
			ch1__.setFrequencyLow(value);
		}
		else if (addr == Address::NR23)
		{
			ch2_.setFrequencyLow(value);
			ch2__.setFrequencyLow(value);
		}
		else if (addr == Address::NR33)
		{
			ch3_.setFrequencyLow(value);
			ch3__.setFrequencyLow(value);
		}

		// Noise

		else if (addr == Address::NR43)
		{
			ch4__.setRandomness(value);
		}

		// APU - Channel Trigger (Update Frequency)

		else if (addr == Address::NR14)
		{
			ch1_.setFrequencyHigh(value);
			ch1__.setFrequencyHigh(value);

			ch1_.setEnableLength((value & 0x40) == 0x40);
			ch1__.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch1_.trigger();
				ch1__.trigger();
			}
		}
		else if (addr == Address::NR24)
		{
			ch2_.setFrequencyHigh(value);
			ch2__.setFrequencyHigh(value);

			ch2_.setEnableLength((value & 0x40) == 0x40);
			ch2__.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch2_.trigger();
				ch2__.trigger();
			}
		}
		else if (addr == Address::NR34)
		{
			ch3_.setFrequencyHigh(value);
			ch3__.setFrequencyHigh(value);

			ch3_.setEnableLength((value & 0x40) == 0x40);
			ch3__.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch3_.trigger();
				ch3__.trigger();
			}
		}
		else if (addr == Address::NR44)
		{
			ch4_.setEnableLength((value & 0x40) == 0x40);
			ch4__.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch4_.trigger();
				ch4__.trigger();
			}
		}
	}

	uint8 APU::getChannelsEnabledState() const
	{
		return
			((uint8)ch1__.getEnable() << 0) |
			((uint8)ch2__.getEnable() << 1) |
			((uint8)ch3__.getEnable() << 2) |
			((uint8)ch4__.getEnable() << 3);
	}

	APUStreamBufferState APU::getBufferState() const
	{
		return APUStreamBufferState{ apuStream_->bufferRemain(), apuStream_->bufferMaxSize() };
	}
}
