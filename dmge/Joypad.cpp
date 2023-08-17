#include "stdafx.h"
#include "Joypad.h"
#include "Memory.h"
#include "Address.h"
#include "AppConfig.h"
#include "InputMapping.h"

namespace dmge
{
	Joypad::Joypad(Memory* mem)
		: mem_{ mem }
	{
	}

	void Joypad::writeRegister(uint8 value)
	{
		// MLT_REQ によるマルチコントローラの設定後、
		// P14 と P15 を HIGH にして P10-13 を読み取るとコントローラの ID が得られる
		// ID は自動的にインクリメントされる

		if ((selected_ & 0x30) != 0x30 && (value & 0x30) == 0x30)
		{
			const int id = 0xf - joypadId_;
			const int newId = (id + (readyForSwitchJoypad_ ? 1 : 0)) % (playerCount_ + 1);
			readyForSwitchJoypad_ = true;
			joypadId_ = 0xf - newId;
		}

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

	void Joypad::setMapping(const InputMapping& keyMap, const InputMapping& gamepadMap)
	{
		// キーボード

		inputRight_ = keyMap.get(JoypadButtons::Right);
		inputLeft_ = keyMap.get(JoypadButtons::Left);
		inputUp_ = keyMap.get(JoypadButtons::Up);
		inputDown_ = keyMap.get(JoypadButtons::Down);
		inputA_ = keyMap.get(JoypadButtons::A);
		inputB_ = keyMap.get(JoypadButtons::B);
		inputSelect_ = keyMap.get(JoypadButtons::Select);
		inputStart_ = keyMap.get(JoypadButtons::Start);

		// 汎用ゲームパッド

		if (const auto gamepad = Gamepad(0); gamepad.isConnected())
		{
			inputRight_ = inputRight_ | gamepad.povRight;
			inputLeft_ = inputLeft_ | gamepad.povLeft;
			inputUp_ = inputUp_ | gamepad.povUp;
			inputDown_ = inputDown_ | gamepad.povDown;

			if (gamepad.buttons.size() > gamepadMap.get(JoypadButtons::A).code())
				inputA_ = inputA_ | gamepadMap.get(JoypadButtons::A);

			if (gamepad.buttons.size() > gamepadMap.get(JoypadButtons::B).code())
				inputB_ = inputB_ | gamepadMap.get(JoypadButtons::B);

			if (gamepad.buttons.size() > gamepadMap.get(JoypadButtons::Select).code())
				inputSelect_ = inputSelect_ | gamepadMap.get(JoypadButtons::Select);

			if (gamepad.buttons.size() > gamepadMap.get(JoypadButtons::Start).code())
				inputStart_ = inputStart_ | gamepadMap.get(JoypadButtons::Start);
		}
	}

	void Joypad::setPlayerCount(int count)
	{
		playerCount_ = count;
	}
}
