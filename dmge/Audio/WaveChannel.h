#pragma once

#include "Channel.h"
#include "LengthCounter.h"

namespace dmge
{
	class Memory;

	class WaveChannel : public Channel
	{
	public:
		uint8 readRegister(uint16 addr) const;

		void writeWaveData(uint16 addr, uint8 value);

		uint8 readWaveData(uint16 addr) const;

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
		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Waveform frequency
		int freq_ = 0;

		// Wave RAM offset (0-31)
		int waveRAMOffset_ = 0;

		// Wave volume
		int waveOutputLevel_ = 0;

		// Wave data
		std::array<uint8, 16> waveData_{};
	};
}
