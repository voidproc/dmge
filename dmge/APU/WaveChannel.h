#pragma once

#include "Channel.h"
#include "LengthCounter.h"

namespace dmge
{
	class Memory;

	class WaveChannel : public Channel
	{
	public:
		WaveChannel(Memory* mem);

		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Frequency

		void setFrequencyLow(uint8 freqLow);

		void setFrequencyHigh(uint8 freqHigh);

		int getFrequency();

		// Wave volume

		void setWaveOutputLevel(uint8 NRx2);

		void resetWaveRAMOffset();

	private:
		Memory* mem_;

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Waveform frequency
		int freq_ = 0;

		// Wave RAM offset (0-31)
		int waveRAMOffset_ = 0;

		// Wave volume
		int waveOutputLevel_ = 0;
	};
}
