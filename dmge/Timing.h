#pragma once

namespace dmge
{
	inline constexpr int ClockFrequency = 4194304;  //0x400000

	inline constexpr int FPS = 60;


	class FPSKeeper
	{
	public:
		FPSKeeper();

		void setEnable(bool enable);

		void sleep(double fps = 60.0);

	private:
		uint64 last_ = 0;
		bool enabled_ = true;
	};
}
