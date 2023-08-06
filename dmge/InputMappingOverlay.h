#pragma once

#include "JoypadButtons.h"

namespace dmge
{
	class InputMappingOverlay
	{
	public:
		static Input Get(InputDeviceType deviceType, JoypadButtons button);
	};
}
