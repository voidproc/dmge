#include "Joypad.h"
#include "Memory.h"
#include "Address.h"

namespace dmge
{
	Joypad::Joypad(Memory* mem)
		: mem_{ mem }
	{
	}

	void Joypad::update()
	{
		uint8 value = 0;

		if (selected_ == SelectedButtons::Direction)
		{
			value |= (KeyRight.pressed() ? 0 : 1) << 0;
			value |= (KeyLeft.pressed() ? 0 : 1) << 1;
			value |= (KeyUp.pressed() ? 0 : 1) << 2;
			value |= (KeyDown.pressed() ? 0 : 1) << 3;
			value |= 0b11100000;
		}
		else
		{
			value |= (KeyX.pressed() ? 0 : 1) << 0;
			value |= (KeyZ.pressed() ? 0 : 1) << 1;
			value |= (KeyBackspace.pressed() ? 0 : 1) << 2;
			value |= (KeyEnter.pressed() ? 0 : 1) << 3;
			value |= 0b11010000;
		}

		mem_->writeDirect(Address::JOYP, value);
	}

	void Joypad::update(uint8 value)
	{
		if ((value & (1 << 4)) == 0)
		{
			selected_ = SelectedButtons::Direction;
		}
		else if ((value & (1 << 5)) == 0)
		{
			selected_ = SelectedButtons::Action;
		}

		update();
	}
}
