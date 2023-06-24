#include "VolumeEnvelope.h"

namespace dmge
{
	void VolumeEnvelope::step()
	{
		if (period_)
		{
			if (periodTimer_ > 0) periodTimer_--;

			if (periodTimer_ == 0)
			{
				periodTimer_ = period_;

				if ((currentVolume_ < 0xf && direction_ == 1) || (currentVolume_ > 0 && direction_ == 0))
				{
					currentVolume_ += (direction_ == 1) ? 1 : -1;
				}
			}
		}
	}

	void VolumeEnvelope::trigger()
	{
		currentVolume_ = initialVolume_;
		periodTimer_ = period_;
	}

	void VolumeEnvelope::set(uint8 NRx2)
	{
		initialVolume_ = NRx2 >> 4;
		direction_ = (NRx2 >> 3) & 1;
		period_ = NRx2 & 0b111;
	}

	uint8 VolumeEnvelope::get() const
	{
		return (initialVolume_ << 4) | (direction_ << 3) | period_;
	}

	int VolumeEnvelope::volume() const
	{
		return currentVolume_;
	}

	int VolumeEnvelope::initialVolume() const
	{
		return initialVolume_;
	}
}
