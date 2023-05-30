#include "APU.h"
#include "Memory.h"
#include "Address.h"

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
		for (const auto i : step(samplesToWrite))
		{
			*(left++) = wave_[posRead_].left;
			*(right++) = wave_[posRead_].right;

			posRead_ = (posRead_ + 1) % wave_.size();
		}

		bufferSize_ -= samplesToWrite;
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
		wave_[posPushed_].left = left;
		wave_[posPushed_].right = right;

		posPushed_ = (posPushed_ + 1) % wave_.size();

		bufferSize_++;
	}

	int APUStream::bufferSize() const
	{
		return bufferSize_;
	}


	APU::APU(Memory* mem)
		:
		mem_{ mem },
		ch1_{ mem, Channels::Ch1 },
		ch2_{ mem, Channels::Ch2 },
		ch3_{ mem, Channels::Ch3 },
		ch4_{ mem, Channels::Ch4 },
		apuStream_{ std::make_shared<APUStream>() },
		audio_{ apuStream_ }
	{
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

			apuStream_->pushSample(sample, sample);

			if (apuStream_->bufferSize() > 300)
			{
				audio_.play();
			}

			if (apuStream_->bufferSize() < 30)
			{
				audio_.pause();
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
