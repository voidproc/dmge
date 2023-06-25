#pragma once

namespace dmge
{
	class Channel;

	class LengthCounter
	{
	public:
		LengthCounter(Channel* channel);

		void step();

		void trigger(int newLengthTimer);

		void setEnable(bool enable);

		bool getEnable() const;

		void setLengthTimer(int value);

		void setExtraLengthClockCond(bool cond);

	private:
		Channel* channel_;

		bool enabled_ = false;
		int lengthTimer_ = 0;
		bool extraLengthClockCond_ = false;
	};
}
