#include "../stdafx.h"
#include "VolumeEnvelope.h"

namespace dmge
{
	void VolumeEnvelope::step()
	{
		if (finished_) return;

		if (period_)
		{
			if (periodTimer_ > 0) periodTimer_--;

			if (periodTimer_ == 0)
			{
				periodTimer_ = (period_ != 0) ? period_ : 8;

				if ((currentVolume_ < 0xf && direction_ == 1) || (currentVolume_ > 0 && direction_ == 0))
				{
					currentVolume_ += (direction_ == 1) ? 1 : -1;
				}

				if (currentVolume_ == 0 || currentVolume_ == 15)
				{
					finished_ = true;
				}
			}
		}
	}

	void VolumeEnvelope::trigger()
	{
		currentVolume_ = initialVolume_;
		periodTimer_ = (period_ != 0) ? period_ : 8;
		finished_ = false;
	}

	void VolumeEnvelope::set(uint8 NRx2)
	{
		const int initialVolume = NRx2 >> 4;
		const int direction = (NRx2 >> 3) & 1;
		const int period = NRx2 & 0b111;

		// TODO: Zombie mode

		initialVolume_ = initialVolume;
		direction_ = direction;
		period_ = period;
	}

	uint8 VolumeEnvelope::get() const
	{
		return (initialVolume_ << 4) | (direction_ << 3) | period_;
	}

	int VolumeEnvelope::volume() const
	{
		if (period_ > 0)
			return currentVolume_;
		else
			return initialVolume_;
	}

	int VolumeEnvelope::initialVolume() const
	{
		return initialVolume_;
	}
}
