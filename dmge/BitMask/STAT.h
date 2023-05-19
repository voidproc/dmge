#pragma once

namespace dmge
{
	namespace BitMask
	{
		namespace STAT
		{
			enum : uint8
			{
				LYCInterruptEnable = 1 << 6,
				OAMInterruptEnable = 1 << 5,
				VBlankInterruptEnable = 1 << 4,
				HBlankInterruptEnable = 1 << 3,
				LYCFlag = 1 << 2,
				ModeFlag = 0b11,
			};
		}
	}
}
