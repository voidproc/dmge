#include "APU.h"
#include "APUStream.h"
#include "../Memory.h"
#include "../Address.h"
#include "../Timing.h"

namespace dmge
{
	APU::APU(Memory* mem, int sampleRate)
		:
		mem_{ mem },
		sampleRate_{ sampleRate },
		apuStream_{ std::make_shared<APUStream>() },
		audio_{ apuStream_ },
		ch1_{},
		ch2_{},
		ch3_{},
		ch4_{},
		frameSeq_{}
	{
	}

	void APU::setCGBMode(bool value)
	{
		cgbMode_ = value;
	}

	int APU::run()
	{
		// マスタースイッチがOffならAPUを停止する

		if (not masterSwitch_)
		{
			audio_.pause();
			return 0;
		}

		// 各チャンネルの Frequency Timer を進める

		ch1_.step();
		ch2_.step();
		ch3_.step();
		ch4_.step();

		// Frame Sequencer

		frameSeq_.step(mem_->read(Address::DIV));

		const bool onExtraLengthClock = frameSeq_.onExtraLengthClock();
		ch1_.setExtraLengthClockCondition(onExtraLengthClock);
		ch2_.setExtraLengthClockCondition(onExtraLengthClock);
		ch3_.setExtraLengthClockCondition(onExtraLengthClock);
		ch4_.setExtraLengthClockCondition(onExtraLengthClock);

		// スイープ

		if (frameSeq_.onSweepClock())
		{
			ch1_.stepSweep();
		}

		// Length control

		if (frameSeq_.onLengthClock())
		{
			ch1_.stepLength();
			ch2_.stepLength();
			ch3_.stepLength();
			ch4_.stepLength();
		}

		// エンベロープ

		if (frameSeq_.onVolumeClock())
		{
			ch1_.stepEnvelope();
			ch2_.stepEnvelope();
			ch4_.stepEnvelope();
		}

		// Output audio
		// (CPUFreq / SampleRate) ==> 4194304 / 44100 ==> Every 95.1 T-cycles

		cycles_ += 1.0;

		if (cycles_ >= 1.0 * ClockFrequency / sampleRate_)
		{
			cycles_ -= 1.0 * ClockFrequency / sampleRate_;

			const std::array<int, 4> chAmp = {
				1 * ch1_.amplitude() * ch1_.getEnable(),
				1 * ch2_.amplitude() * ch2_.getEnable(),
				1 * ch3_.amplitude() * ch3_.getEnable(),
				1 * ch4_.amplitude() * ch4_.getEnable(),
			};

			// Input / Panning

			std::array<int, 4> leftInput = { 0, 0, 0, 0 };
			std::array<int, 4> rightInput = { 0, 0, 0, 0 };

			for (int i : step(4))
			{
				if ((nr51_ >> i) & 1) rightInput[i] = chAmp[i];
				if ((nr51_ >> (i + 4)) & 1) leftInput[i] = chAmp[i];
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

			const double leftVolume = (((nr50_ >> 4) & 0b111) + 1) / 8.0;
			const double rightVolume = (((nr50_ >> 0) & 0b111) + 1) / 8.0;

			// バッファが十分なら書き込まない

			if (apuStream_->bufferRemain() > sampleRate_ / 8)
			{
				return 0;
			}

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
			masterSwitch_ = (value & 0x80) != 0;

			// APUがoffになったとき、APUレジスタが全てクリアされる
			// (except on the DMG, where length counters are unaffected by power and can still be written while off).
			if ((value & 0x80) == 0)
			{
				for (uint16 apuReg = Address::NR10; apuReg <= Address::NR51; apuReg++)
				{
					if (not cgbMode_)
					{
						switch (apuReg)
						{
						case Address::NR11:
							ch1_.setDuty(0);
							mem_->writeDirect(apuReg, mem_->read(apuReg) & 0x3f);
							break;

						case Address::NR21:
							ch2_.setDuty(0);
							mem_->writeDirect(apuReg, mem_->read(apuReg) & 0x3f);
							break;

						case Address::NR31:
						case Address::NR41:
							break;

						default:
							mem_->write(apuReg, 0);
						}
					}
				}
			}
			else
			{
				// When powered on:
				// - the frame sequencer is reset so that the next step will be 0
				// - the square duty units are reset to the first step of the waveform
				// - the wave channel's sample buffer is reset to 0
				frameSeq_.reset();
				ch1_.resetDutyPosition();
				ch3_.resetWaveRAMOffset(); // ?
			}
		}

		// Sweep

		else if (addr == Address::NR10)
		{
			ch1_.setSweep(value);
		}

		// APU - Reload Length Timer

		else if (addr == Address::NR11)
		{
			ch1_.setLengthTimer(value);
			ch1_.setDuty(value);
		}
		else if (addr == Address::NR21)
		{
			ch2_.setLengthTimer(value);
			ch2_.setDuty(value);
		}
		else if (addr == Address::NR31)
		{
			ch3_.setLengthTimer(value);
		}
		else if (addr == Address::NR41)
		{
			ch4_.setLengthTimer(value);
		}

		// APU - Enable DAC / Envelope

		else if (addr == Address::NR12)
		{
			ch1_.setEnvelope(value);

			bool dacEnable = (value & 0xf8) != 0;
			ch1_.setDACEnable(dacEnable);

			if (not dacEnable)
			{
				ch1_.setEnable(false);
			}
		}
		else if (addr == Address::NR22)
		{
			ch2_.setEnvelope(value);

			bool dacEnable = (value & 0xf8) != 0;
			ch2_.setDACEnable(dacEnable);

			if (not dacEnable)
			{
				ch2_.setEnable(false);
			}
		}
		else if (addr == Address::NR30)
		{
			bool dacEnable = (value >> 7) != 0;
			ch3_.setDACEnable(dacEnable);

			if (not dacEnable)
			{
				ch3_.setEnable(false);
			}
		}
		else if (addr == Address::NR42)
		{
			ch4_.setEnvelope(value);

			bool dacEnable = (value & 0xf8) != 0;
			ch4_.setDACEnable(dacEnable);

			if (not dacEnable)
			{
				ch4_.setEnable(false);
			}
		}

		// Wave channel volume

		else if (addr == Address::NR32)
		{
			ch3_.setWaveOutputLevel(value);
		}

		// APU - Update Frequency

		else if (addr == Address::NR13)
		{
			ch1_.setFrequencyLow(value);
		}
		else if (addr == Address::NR23)
		{
			ch2_.setFrequencyLow(value);
		}
		else if (addr == Address::NR33)
		{
			ch3_.setFrequencyLow(value);
		}

		// Noise

		else if (addr == Address::NR43)
		{
			ch4_.setRandomness(value);
		}

		// APU - Channel Trigger (Update Frequency)

		else if (addr == Address::NR14)
		{
			ch1_.setFrequencyHigh(value);

			ch1_.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch1_.trigger();
			}
		}
		else if (addr == Address::NR24)
		{
			ch2_.setFrequencyHigh(value);

			ch2_.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch2_.trigger();
			}
		}
		else if (addr == Address::NR34)
		{
			ch3_.setFrequencyHigh(value);

			ch3_.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch3_.trigger();
			}
		}
		else if (addr == Address::NR44)
		{
			ch4_.setEnableLength((value & 0x40) == 0x40);

			if (value & 0x80)
			{
				ch4_.trigger();
			}
		}
		else if (addr == Address::NR50)
		{
			nr50_ = value;
		}
		else if (addr == Address::NR51)
		{
			nr51_ = value;
		}
		else if (addr >= Address::WaveRAM && addr <= Address::WaveRAM + 15)
		{
			ch3_.writeWaveData(addr, value);
		}
	}

	uint8 APU::readRegister(uint16 addr)
	{
		static constexpr std::array<uint8, 48> RegisterMasks = {
			0x80,0x3F,0x00,0xFF,0xBF,
			0xFF,0x3F,0x00,0xFF,0xBF,
			0x7F,0xFF,0x9F,0xFF,0xBF,
			0xFF,0xFF,0x00,0x00,0xBF,
			0x00,0x00,0x70,
			0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		};

		const uint8 mask = RegisterMasks[addr - Address::NR10];

		if (addr == Address::NR50)
		{
			return nr50_;
		}
		else if (addr == Address::NR51)
		{
			return nr51_;
		}
		else if (addr == Address::NR52)
		{
			return ((uint8)masterSwitch_ << 7) | getChannelsEnabledState() | mask;
		}
		else if (addr >= Address::NR10 && addr <= Address::NR14)
		{
			return ch1_.readRegister(addr) | mask;
		}
		else if (addr >= Address::NR21 && addr <= Address::NR24)
		{
			return ch2_.readRegister(addr) | mask;
		}
		else if (addr >= Address::NR30 && addr <= Address::NR34)
		{
			return ch3_.readRegister(addr) | mask;
		}
		else if (addr >= Address::NR41 && addr <= Address::NR44)
		{
			return ch4_.readRegister(addr) | mask;
		}
		else if (addr >= Address::WaveRAM && addr <= Address::WaveRAM + 15)
		{
			return ch3_.readRegister(addr) | mask;
		}

		return mask;
	}

	uint8 APU::getChannelsEnabledState() const
	{
		return
			((uint8)ch1_.getEnable() << 0) |
			((uint8)ch2_.getEnable() << 1) |
			((uint8)ch3_.getEnable() << 2) |
			((uint8)ch4_.getEnable() << 3);
	}

	APUStreamBufferState APU::getBufferState() const
	{
		return APUStreamBufferState{ apuStream_->bufferRemain(), apuStream_->bufferMaxSize() };
	}
}
