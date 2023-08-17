#include "stdafx.h"
#include "InputMappingOverlay.h"

namespace dmge
{
	Input InputMappingOverlay::Get(InputDeviceType deviceType, JoypadButtons button)
	{
		const StringView deviceName = deviceType == InputDeviceType::Keyboard ? U"key"_sv : U"gamepad"_sv;

		while (System::Update())
		{
			Scene::Rect().draw(ColorF{ 0.1 });
			FontAsset(U"menu")(U"Press the {} for {} button..."_fmt(deviceName, ToString(button))).drawAt(Scene::CenterF(), Palette::Khaki);
			FontAsset(U"menu")(U"[ESC][R-Click]: Back"_fmt(deviceName, ToString(button))).drawAt(Scene::CenterF().movedBy(0, 24));

			if (deviceType == InputDeviceType::Keyboard)
			{
				const auto& allInputs = Keyboard::GetAllInputs();

				if (not allInputs.isEmpty())
				{
					const uint8 code = allInputs[0].code();

					if (code == KeyEscape.code())
					{
						return Input{};
					}
					else if (code != KeyAlt.code())
					{
						return allInputs[0];
					}
				}

				if (MouseR.up())
				{
					return Input{};
				}
			}
			else
			{
				if (const auto gamepad = Gamepad(0); Gamepad(0).isConnected())
				{
					for (const auto [index, btn] : Indexed(gamepad.buttons))
					{
						if (btn.pressed())
						{
							return btn;
						}
					}
				}

				if (MouseR.up() || KeyEscape.up())
				{
					return Input{};
				}
			}
		}

		return Input{};
	}
}
