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
		selected_ = value & 0x30;

		if (enabled_)
		{
			update();
		}
	}

	uint8 Joypad::readRegister()
	{
		const auto selected = selected_ & 0x30;

		if (selected == 0x30)
		{
			if (playerCount_ == 0)
			{
				return 0xf0 | (dirState_ | actState_);
			}
			else
			{
				const int id = 0xf - joypadId_;
				const int newId = (id + 1) % (playerCount_ + 1);
				joypadId_ = 0xf - newId;
				return 0xf0 | joypadId_;
			}
		}
		else if (selected == 0x20)
		{
			return 0xc0 | selected | dirState_;
		}
		else if (selected == 0x10)
		{
			return 0xc0 | selected | actState_;
		}

		return 0xff;
	}

	void Joypad::update()
	{
		bool inputRight = inputRight_.pressed();
		bool inputLeft = inputLeft_.pressed();
		bool inputUp = inputUp_.pressed();
		bool inputDown = inputDown_.pressed();

		if (const auto procon = ProController(0); procon.isConnected())
		{
			const auto lstick = procon.LStick();
			inputRight |= lstick.x > 0.5;
			inputLeft |= lstick.x < -0.5;
			inputUp |= lstick.y < -0.5;
			inputDown |= lstick.y > 0.5;
		}

		dirState_ = 0;
		dirState_ |= (inputRight ? 0 : 1) << 0;
		dirState_ |= (inputLeft ? 0 : 1) << 1;
		dirState_ |= (inputUp ? 0 : 1) << 2;
		dirState_ |= (inputDown ? 0 : 1) << 3;

		actState_ = 0;
		actState_ |= (inputA_.pressed() ? 0 : 1) << 0;
		actState_ |= (inputB_.pressed() ? 0 : 1) << 1;
		actState_ |= (inputSelect_.pressed() ? 0 : 1) << 2;
		actState_ |= (inputStart_.pressed() ? 0 : 1) << 3;
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

			return;
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

			return;
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

	void Joypad::setPlayerCount(int count)
	{
		playerCount_ = count;
	}
}
