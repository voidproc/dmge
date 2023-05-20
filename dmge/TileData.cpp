#include "TileData.h"

namespace dmge
{
	namespace TileData
	{
		uint16 GetAddress(uint16 baseAddr, uint8 tileId, uint8 row, bool yFlip)
		{
			const auto yShift = yFlip ? 2 * (7 - row) : 2 * row;
			return baseAddr
				+ (baseAddr == 0x8000 ? tileId : (int8)tileId) * 0x10
				+ yShift;
		}

		uint8 GetColor(uint16 tileData, int dotNth, bool xFlip)
		{
			const auto bitShift = xFlip ? dotNth : 7 - dotNth;
			return ((tileData >> bitShift) & 1) | (((tileData >> (bitShift + 8)) & 1) << 1);
		}
	}
}
