#include "../stdafx.h"
#include "FrameSequencer.h"

namespace dmge
{
	void FrameSequencer::step(uint8 div)
	{
		onVolumeClock_ = false;
		onSweepClock_ = false;
		onLengthClock_ = false;
		onExtraLengthClock_ = false;

		if ((prevDiv_ & 0b10000) && (div & 0b10000) == 0)
		{
			clock_++;

			if ((clock_ % 8) == 7) onVolumeClock_ = true;
			if ((clock_ & 3) == 2) onSweepClock_ = true;
			if ((clock_ % 2) == 0) onLengthClock_ = true;
			if (((clock_ + 1) % 2) != 0) onExtraLengthClock_ = true;
		}

		prevDiv_ = div;
	}

	bool FrameSequencer::onVolumeClock() const
	{
		return onVolumeClock_;
	}

	bool FrameSequencer::onSweepClock() const
	{
		return onSweepClock_;
	}

	bool FrameSequencer::onLengthClock() const
	{
		return onLengthClock_;
	}

	bool FrameSequencer::onExtraLengthClock() const
	{
		return onExtraLengthClock_;
	}

	void FrameSequencer::reset()
	{
		clock_ = -1;
	}
}
