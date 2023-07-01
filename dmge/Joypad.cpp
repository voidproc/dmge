#include "Joypad.h"
#include "Memory.h"
#include "Address.h"
#include "AppConfig.h"

namespace dmge
{
	Joypad::Joypad(Memory* mem)
		: mem_{ mem }
	{
	}

	void Joypad::writeRegister(uint8 value)
	{
		if ((value & (1 << 4)) == 0)
		{
			selected_ = SelectedButtons::Direction;
		}
		else if ((value & (1 << 5)) == 0)
		{
			selected_ = SelectedButtons::Action;
		}

		if (enabled_)
		{
			update();
		}
	}

	uint8 Joypad::readRegister() const
	{
		return joyp_;
	}

	void Joypad::update()
	{
		uint8 value = 0;

		// 入力を統合

		if (selected_ == SelectedButtons::Direction)
		{
			value |= (inputRight_.pressed() ? 0 : 1) << 0;
			value |= (inputLeft_.pressed() ? 0 : 1) << 1;
			value |= (inputUp_.pressed() ? 0 : 1) << 2;
			value |= (inputDown_.pressed() ? 0 : 1) << 3;
			value |= 0b11100000;
		}
		else
		{
			value |= (inputA_.pressed() ? 0 : 1) << 0;
			value |= (inputB_.pressed() ? 0 : 1) << 1;
			value |= (inputSelect_.pressed() ? 0 : 1) << 2;
			value |= (inputStart_.pressed() ? 0 : 1) << 3;
			value |= 0b11010000;
		}

		joyp_ = value;
	}

	void Joypad::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	void Joypad::setButtonAssign(const GamepadButtonAssign& gamepadButtonAssign)
	{
		// キーボード

		inputRight_ = KeyRight;
		inputLeft_ = KeyLeft;
		inputUp_ = KeyUp;
		inputDown_ = KeyDown;
		inputA_ = KeyX;
		inputB_ = KeyZ;
		inputSelect_ = KeyBackspace;
		inputStart_ = KeyEnter;

		// Joy-Con(L), Joy-Con(R) （両手持ち）

		const auto joyConL = JoyConL(0);
		const auto joyConR = JoyConR(0);

		if (joyConL.isConnected() && joyConR.isConnected())
		{
			inputRight_ = inputRight_ | joyConL.button3;
			inputLeft_ = inputLeft_ | joyConL.button0;
			inputUp_ = inputUp_ | joyConL.button2;
			inputDown_ = inputDown_ | joyConL.button1;
			inputA_ = inputA_ | joyConR.button0;
			inputB_ = inputB_ | joyConR.button2;
			inputSelect_ = inputSelect_ | joyConR.button3;
			inputStart_ = inputStart_ | joyConR.button1;
		}

		// Proコントローラー

		if (const auto procon = ProController(0); procon.isConnected())
		{
			inputRight_ = inputRight_ | procon.povRight;
			inputLeft_ = inputLeft_ | procon.povLeft;
			inputUp_ = inputUp_ | procon.povUp;
			inputDown_ = inputDown_ | procon.povDown;
			inputA_ = inputA_ | procon.buttonA;
			inputB_ = inputB_ | procon.buttonB;
			inputSelect_ = inputSelect_ | procon.buttonMinus;
			inputStart_ = inputStart_ | procon.buttonPlus;
		}

		// 汎用ゲームパッド

		if (const auto gamepad = Gamepad(0); gamepad.isConnected())
		{
			inputRight_ = inputRight_ | gamepad.povRight;
			inputLeft_ = inputLeft_ | gamepad.povLeft;
			inputUp_ = inputUp_ | gamepad.povUp;
			inputDown_ = inputDown_ | gamepad.povDown;

			if (gamepad.buttons.size() > gamepadButtonAssign.buttonA)
				inputA_ = inputA_ | gamepad.buttons[gamepadButtonAssign.buttonA];

			if (gamepad.buttons.size() > gamepadButtonAssign.buttonB)
				inputB_ = inputB_ | gamepad.buttons[gamepadButtonAssign.buttonB];

			if (gamepad.buttons.size() > gamepadButtonAssign.buttonSelect)
				inputSelect_ = inputSelect_ | gamepad.buttons[gamepadButtonAssign.buttonSelect];

			if (gamepad.buttons.size() > gamepadButtonAssign.buttonStart)
				inputStart_ = inputStart_ | gamepad.buttons[gamepadButtonAssign.buttonStart];
		}
	}
}
