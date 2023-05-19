#pragma once

namespace dmge
{
	namespace BitMask
	{
		namespace InterruptFlag
		{
			enum : uint8
			{
				Joypad = 1 << 4,
				Serial = 1 << 3,
				Timer = 1 << 2,
				STAT = 1 << 1,
				VBlank = 1 << 0,
			};
		}
	}
}
