#pragma once

namespace dmge
{
	class Channel;

	class FrequencySweep
	{
	public:
		FrequencySweep(Channel* channel);

		int step(int freq);

		void trigger(int freq);

		void set(uint8 NRx0);

		uint8 get() const;

	private:
		Channel* channel_;

		int period_ = 0;
		int direction_ = 0;
		int shift_ = 0;
		int sweepTimer_ = 0;
		bool enabled_ = false;
		int shadowFreq_ = 0;

		int calcSweepFrequency_();
	};
}
