#pragma once

#include "Channel.h"
#include "VolumeEnvelope.h"
#include "FrequencySweep.h"
#include "LengthCounter.h"

namespace dmge
{
	class SquareChannel : public Channel
	{
	public:
		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Square duty

		void setDuty(uint8 NRx1);

		void resetDutyPosition();

		// Envelope

		void stepEnvelope();

		void setEnvelope(uint8 NRx2);

		// Sweep

		void stepSweep();

		void setSweep(uint8 NRx0);

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Frequency

		void setFrequencyLow(uint8 freqLow);

		void setFrequencyHigh(uint8 freqHigh);

		int getFrequency();

	private:
		VolumeEnvelope envelope_{};

		FrequencySweep sweep_{ this };

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Duty ratio
		int duty_ = 0;

		// Duty position
		int dutyPos_ = 0;

		// Waveform frequency
		int freq_ = 0;
	};
}
