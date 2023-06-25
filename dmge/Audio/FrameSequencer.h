#pragma once

namespace dmge
{
	class FrameSequencer
	{
	public:
		void step(uint8 div);

		bool onVolumeClock() const;

		bool onSweepClock() const;

		bool onLengthClock() const;

		bool onExtraLengthClock() const;

		void reset();

	private:
		uint64 clock_ = 0;

		uint8 prevDiv_ = 0;

		bool onVolumeClock_ = false;
		bool onSweepClock_ = false;
		bool onLengthClock_ = false;
		bool onExtraLengthClock_ = false;
	};
}
