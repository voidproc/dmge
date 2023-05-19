#pragma once

namespace dmge
{
	class Memory;

	class Timer
	{
	public:
		Timer(Memory* mem);

		void update();

		void resetInternalDIV();

		void abortInterrupt();

	private:
		Memory* mem_;

		uint16 divInternal_ = 0;
		uint8 divBitPrev_ = 0;
		int tmaCount_ = 0;
	};
}
