#pragma once

namespace dmge
{
	namespace BitMask
	{
		namespace LCDC
		{
			enum : uint8
			{
				LCDEnable = 1 << 7,
				WindowTileMapArea = 1 << 6,
				WindowEnable = 1 << 5,
				BGAndWindowTileDataArea = 1 << 4,
				BGTileMapArea = 1 << 3,
				OBJSize = 1 << 2,
				OBJEnable = 1 << 1,
				BGAndWindowEnable = 1 << 0,
			};
		}
	}
}
