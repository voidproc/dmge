#include "FrequencySweep.h"
#include "Channel.h"

namespace dmge
{
	FrequencySweep::FrequencySweep(Channel* channel)
		: channel_{ channel }
	{
	}

	int FrequencySweep::step(int freq)
	{
		if (sweepTimer_ > 0) sweepTimer_--;

		if (sweepTimer_ == 0)
		{
			sweepTimer_ = (period_ > 0) ? period_ : 8;

			if (enabled_ && period_ > 0)
			{
				const int newFreq = calcSweepFrequency_();

				if (newFreq <= 2047 && shift_ > 0)
				{
					freq = shadowFreq_ = newFreq;

					// Check sweepEnabled
					calcSweepFrequency_();
				}
			}
		}

		return freq;
	}

	void FrequencySweep::trigger(int freq)
	{
		shadowFreq_ = freq;
		sweepTimer_ = (period_ > 0) ? period_ : 8;
		enabled_ = period_ != 0 || shift_ != 0;
		if (shift_ != 0) calcSweepFrequency_();
	}

	void FrequencySweep::set(uint8 NRx0)
	{
		period_ = (NRx0 >> 4) & 0b111;
		direction_ = (NRx0 >> 3) & 1;
		shift_ = NRx0 & 0b111;
	}

	int FrequencySweep::calcSweepFrequency_()
	{
		int newFreq = shadowFreq_ >> shift_;

		if (direction_ == 0)
			newFreq = shadowFreq_ + newFreq;
		else
			newFreq = shadowFreq_ - newFreq;

		if (newFreq > 2047) channel_->setEnable(false);
		return newFreq;
	}
}
