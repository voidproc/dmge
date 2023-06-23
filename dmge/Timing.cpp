#include "stdafx.h"
#include "Timing.h"

namespace dmge
{
	FPSKeeper::FPSKeeper()
		: last_{ Time::GetMicrosec() }
	{
	}

	void FPSKeeper::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	void FPSKeeper::sleep(double fps)
	{
		while (enabled_)
		{
			const auto t = Time::GetMicrosec();

			if (t - last_ > 1'000'000 / fps)
			{
				last_ = t;
				return;
			}

			System::Sleep(static_cast<int32>((1000 / fps - (t - last_)) * 0.95));
		}
	}
}
