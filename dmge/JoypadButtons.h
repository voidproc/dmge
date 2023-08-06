#pragma once

namespace dmge
{
	enum class JoypadButtons : uint8
	{
		Right = 0,
		Left = 1,
		Up = 2,
		Down = 3,
		A = 4,
		B = 5,
		Select = 6,
		Start = 7,
	};

	inline constexpr StringView ToString(JoypadButtons button)
	{
		switch (button)
		{
		case JoypadButtons::Right: return U"Right"_sv;
		case JoypadButtons::Left: return U"Left"_sv;
		case JoypadButtons::Up: return U"Up"_sv;
		case JoypadButtons::Down: return U"Down"_sv;
		case JoypadButtons::A: return U"A"_sv;
		case JoypadButtons::B: return U"B"_sv;
		case JoypadButtons::Select: return U"Select"_sv;
		case JoypadButtons::Start: return U"Start"_sv;
		}

		return U""_sv;
	}
}
