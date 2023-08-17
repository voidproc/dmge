#include "stdafx.h"
#include "Timing.h"

namespace dmge
{
	FPSKeeper::FPSKeeper()
	{
		next_ = 1000.0 / 60.0;
	}

	void FPSKeeper::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	void FPSKeeper::sleep(double fps)
	{
		// あまりに差がある場合はリセット
		if (Abs(sw_.msF() - next_) > 1000.0)
		{
			next_ = sw_.msF() + 1000.0 / fps;
		}

		while (enabled_ && sw_.msF() < next_)
		{
			System::Sleep(1);
		}

		next_ += 1000.0 / fps;
	}
}
