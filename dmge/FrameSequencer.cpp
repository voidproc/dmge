#include "FrameSequencer.h"

namespace dmge
{
	void FrameSequencer::step(uint8 div)
	{
		if ((prevDiv_ & 0b10000) && (div & 0b10000) == 0)
		{
			clock_++;
		}

		prevDiv_ = div;
	}

	bool FrameSequencer::onVolumeClock() const
	{
		return (clock_ % 8) == 7;
	}

	bool FrameSequencer::onSweepClock() const
	{
		return (clock_ & 3) == 2;
	}

	bool FrameSequencer::onLengthClock() const
	{
		return (clock_ & 3) == 2;
	}

	bool FrameSequencer::onExtraLengthClock() const
	{
		return ((clock_ + 1) % 2) != 0;
	}
}
