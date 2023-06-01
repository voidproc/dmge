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

	int APUStream::bufferRemain() const
	{
		return bufferSize_;
	}

	int APUStream::bufferTotalSize() const
	{
		return wave_.size();
	}

	std::pair<int, int> APUStream::getSamplePos()
	{
		return std::pair<int, int>(posPushed_, posRead_);
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
		const uint8 NR52 = mem_->read(Address::NR52);
		const uint8 masterSwitch = NR52 >> 7;
		if (masterSwitch == 0) return;

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

		if (cycles_ == 0/* && apuStream_->bufferRemain() < 8000*/)
		{
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
		}

		if (apuStream_->bufferRemain() > 8000 && not audio_.isPlaying())
		{
			audio_.play();
		}

		if (apuStream_->bufferRemain() <= 3000 && audio_.isPlaying())
		{
			audio_.pause();
		}

		cycles_ = (cycles_ + 1) % 95;
	}

	void APU::pause()
	{
		audio_.pause();
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

	void APU::draw(const Point& pos)
	{
		const Size GaugeSize{ 240, 12 };

		const auto [pushedPos, readPos] = apuStream_->getSamplePos();
		const auto w = (pushedPos >= readPos) ? pushedPos - readPos : pushedPos + apuStream_->bufferTotalSize() - readPos;

		if (apuStream_->bufferRemain() > 0)
		{
			RectF{ pos, SizeF{ 1.0 * GaugeSize.x * w / apuStream_->bufferTotalSize(), 12} }.draw(Palette::Green);
		}
		else
		{
			RectF{ pos, GaugeSize }.draw(Palette::Red);
		}

		Rect{ pos, GaugeSize }.drawFrame(1.0);

	}
}
