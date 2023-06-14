#pragma once

#include "Channel.h"
#include "VolumeEnvelope.h"
#include "LengthCounter.h"

namespace dmge
{
	class NoiseChannel : public Channel
	{
	public:
		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Envelope

		void stepEnvelope();

		void setEnvelope(uint8 NRx2);

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Noise

		void setRandomness(uint8 NRx3);

	private:
		VolumeEnvelope envelope_{};

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Noise randomness

		int divisorShift_ = 0;
		int counterWidth_ = 0;
		int divisor_ = 0;
		uint16 lfsr_ = 0;
	};
}
