#include "WaveChannel.h"
#include "Frequency.h"
#include "../Memory.h"

namespace dmge
{
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
		if (getDACEnable())
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
}
