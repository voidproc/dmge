#pragma once

namespace dmge
{
	enum class PPUMode : uint8
	{
		// Mode 2
		OAMScan = 2,

		// Mode 3
		Drawing = 3,

		// Mode 0
		HBlank = 0,

		// Mode 1
		VBlank = 1,
	};
}
