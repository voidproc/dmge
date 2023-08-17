#include "../stdafx.h"
#include "Frequency.h"

namespace dmge
{
	int FrequencyReplacedLower(int originalFreq, uint8 lower)
	{
		return (originalFreq & 0x700) | lower;
	}

	int FrequencyReplacedHigher(int originalFreq, uint8 higher)
	{
		return ((higher & 0x7) << 8) | (originalFreq & 0xff);
	}
}
