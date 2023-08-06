#pragma once

#include "JoypadButtons.h"
#include "InputMappingArray.h"

namespace dmge
{
	// Joypad の各ボタンに対する、キーボード・ゲームパッド等の入力の割り当て
	class InputMapping
	{
	public:
		InputMapping(InputDeviceType deviceType);

		// Joypad のボタン `button` に対し入力オブジェクトを割り当てる
		void set(JoypadButtons button, Input key);

		// Joypad の各ボタンに対し入力オブジェクト（配列で指定）を割り当てる
		void set(const InputMappingArray& mapping);

		// Joypad のボタン `button` に割り当てられている入力オブジェクト
		const Input& get(JoypadButtons button) const;

		// Joypad の各ボタンに割り当てられている入力オブジェクトの配列
		const InputMappingArray& get() const;

	private:
		InputMappingArray inputMap_{};
	};
}
