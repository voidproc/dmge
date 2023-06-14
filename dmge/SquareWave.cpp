#include "SquareWave.h"

namespace dmge
{
	int SquareWaveAmplitude(int duty, int dutyPos)
	{
		constexpr std::array<uint8, 4> waveDutyTable = {
			0b00000001, 0b00000011, 0b00001111, 0b11111100,
		};

		return (waveDutyTable[duty] >> (7 - dutyPos)) & 1;
	}
}
