#pragma once

namespace dmge
{
	class VolumeEnvelope
	{
	public:
		void step();

		void trigger();

		void set(uint8 NRx2);

		uint8 get() const;

		int volume() const;

		int initialVolume() const;

	private:
		int initialVolume_ = 0;
		int direction_ = 0;
		int period_ = 0;
		int currentVolume_ = 0;
		int periodTimer_ = 0;
	};
}
