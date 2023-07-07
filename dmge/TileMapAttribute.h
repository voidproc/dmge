#pragma once

namespace dmge
{
	// (CGB) 背景マップ属性
	union TileMapAttribute
	{
		struct
		{
			uint8 palette : 3;
			uint8 bank : 1;
			uint8 unused1 : 1;
			bool xFlip : 1;
			bool yFlip : 1;
			uint8 priority : 1;
		} attr;
		uint8 value;

		TileMapAttribute(uint8 val = 0)
			: value{ val }
		{
		}
	};
}
