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
		audio_{ apuStream_ }
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

		// Save previous state
		prevDiv_ = div;

		// Output audio
		// (CPUFreq / SampleRate) ==> 4194304 / 44100 ==> Every 95.1 T-cycles

		cycles_ += 1.0;

		if (cycles_ >= 1.0 * ClockFrequency / sampleRate_)
		{
			cycles_ -= 1.0 * ClockFrequency / sampleRate_;

			const std::array<int, 4> chAmp = {
				ch1_.amplitude() * ch1_.getEnable(),
				ch2_.amplitude() * ch2_.getEnable(),
				ch3_.amplitude() * ch3_.getEnable(),
				ch4_.amplitude() * ch4_.getEnable(),
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

	void APU::setEnable(Channels ch, bool enable)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setEnable(enable); return;
		case Channels::Ch2: ch2_.setEnable(enable); return;
		case Channels::Ch3: ch3_.setEnable(enable); return;
		case Channels::Ch4: ch4_.setEnable(enable); return;
		}
	}

	uint8 APU::getEnableMask() const
	{
		return
			((uint8)ch1_.getEnable() << 0) |
			((uint8)ch2_.getEnable() << 1) |
			((uint8)ch3_.getEnable() << 2) |
			((uint8)ch4_.getEnable() << 3);
	}

	void APU::setFrequency(Channels ch, int freq)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setFrequency(freq); return;
		case Channels::Ch2: ch2_.setFrequency(freq); return;
		case Channels::Ch3: ch3_.setFrequency(freq); return;
		}
	}

	void APU::setFrequencyLow(Channels ch, uint8 value)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setFrequency(value | (ch1_.getFrequency() & 0x700)); return;
		case Channels::Ch2: ch2_.setFrequency(value | (ch2_.getFrequency() & 0x700)); return;
		case Channels::Ch3: ch3_.setFrequency(value | (ch3_.getFrequency() & 0x700)); return;
		}
	}

	void APU::setFrequencyHigh(Channels ch, uint8 value)
	{
		switch (ch)
		{
		case Channels::Ch1: ch1_.setFrequency(((value & 0x7) << 8) | (ch1_.getFrequency() & 0xff)); return;
		case Channels::Ch2: ch2_.setFrequency(((value & 0x7) << 8) | (ch2_.getFrequency() & 0xff)); return;
		case Channels::Ch3: ch3_.setFrequency(((value & 0x7) << 8) | (ch3_.getFrequency() & 0xff)); return;
		}
	}

	void APU::setEnvelopeAndDAC(Channels ch, uint8 reg)
	{
		bool dacState = (reg & 0xf8) != 0;

		// Enabling DAC shouldn't re-enable channel
		if (dacState) return;

		switch (ch)
		{
		case Channels::Ch1: ch1_.setEnable(dacState); return;
		case Channels::Ch2: ch2_.setEnable(dacState); return;
		case Channels::Ch4: ch4_.setEnable(dacState); return;
		}
	}

	void APU::setDAC(Channels ch, bool enable)
	{
		// Enabling DAC shouldn't re-enable channel
		if (enable) return;

		switch (ch)
		{
		case Channels::Ch3: ch3_.setEnable(enable); return;
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

	APUStreamBufferState APU::getBufferState() const
	{
		return APUStreamBufferState{ apuStream_->bufferRemain(), apuStream_->bufferMaxSize() };
	}
}
