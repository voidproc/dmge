#include "stdafx.h"
#include "InputMapping.h"

namespace dmge
{
	InputMapping::InputMapping(InputDeviceType deviceType)
	{
		if (deviceType == InputDeviceType::Keyboard)
		{
			inputMap_ = { KeyRight, KeyLeft, KeyUp, KeyDown, KeyX, KeyZ, KeyBackspace, KeyEnter };
		}
		else
		{
			inputMap_ = { Input{}, Input{}, Input{}, Input{},
				Input{ InputDeviceType::Gamepad, 0 },
				Input{ InputDeviceType::Gamepad, 1 },
				Input{ InputDeviceType::Gamepad, 2 },
				Input{ InputDeviceType::Gamepad, 3 },
			};
		}
	}

	void InputMapping::set(JoypadButtons button, Input key)
	{
		inputMap_[FromEnum(button)] = key;
	}

	void InputMapping::set(const InputMappingArray& mapping)
	{
		for (const auto [index, button] : Indexed(mapping))
		{
			inputMap_[index] = button;
		}
	}

	const Input& InputMapping::get(JoypadButtons button) const
	{
		return inputMap_[FromEnum(button)];
	}

	const InputMappingArray& InputMapping::get() const
	{
		return inputMap_;
	}
}
