#pragma once

namespace dmge
{
	namespace SGB
	{
		// Screen mask (MASK_EN)
		enum class MaskMode : uint8
		{
			None = 0,
			Freeze = 1,
			Black = 2,
			Color0 = 3,
		};
	}
}
