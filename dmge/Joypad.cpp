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

		// キーボード

		InputGroup inputRight = KeyRight;
		InputGroup inputLeft = KeyLeft;
		InputGroup inputUp = KeyUp;
		InputGroup inputDown = KeyDown;
		InputGroup inputA = KeyX;
		InputGroup inputB = KeyZ;
		InputGroup inputSelect = KeyBackspace;
		InputGroup inputStart = KeyEnter;

		// ジョイコンL

		if (const auto joyConL = JoyConL(0); joyConL.isConnected())
		{
			inputRight = inputRight | joyConL.button3;
			inputLeft = inputLeft | joyConL.button0;
			inputUp = inputUp | joyConL.button2;
			inputDown = inputDown | joyConL.button1;
		}

		// ジョイコンR

		if (const auto joyConR = JoyConR(0); joyConR.isConnected())
		{
			inputA = inputA | joyConR.button0;
			inputB = inputB | joyConR.button2;
			inputSelect = inputSelect | joyConR.button3;
			inputStart = inputStart | joyConR.button1;
		}

		if (selected_ == SelectedButtons::Direction)
		{
			value |= (inputRight.pressed() ? 0 : 1) << 0;
			value |= (inputLeft.pressed() ? 0 : 1) << 1;
			value |= (inputUp.pressed() ? 0 : 1) << 2;
			value |= (inputDown.pressed() ? 0 : 1) << 3;
			value |= 0b11100000;
		}
		else
		{
			value |= (inputA.pressed() ? 0 : 1) << 0;
			value |= (inputB.pressed() ? 0 : 1) << 1;
			value |= (inputSelect.pressed() ? 0 : 1) << 2;
			value |= (inputStart.pressed() ? 0 : 1) << 3;
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
