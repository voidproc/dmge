#pragma once

namespace dmge
{
	class Memory;

	struct GamepadButtonAssign;

	class Joypad
	{
	public:
		Joypad(Memory* mem);

		// IOレジスタへの書き込み
		// 選択状態（方向orアクション）とIOレジスタを更新する
		void writeRegister(uint8 value);

		// IOレジスタからの読み込み
		uint8 readRegister();

		// 現在の選択状態（方向orアクション）に応じてIOレジスタを更新する
		void update();

		// ジョイパッド有効／無効を切り替える
		// 無効の場合、update()が呼ばれてもデバイスからボタンの状態を取得しない
		void setEnable(bool enable);

		// ボタン割り当てを設定
		void setButtonAssign(const GamepadButtonAssign& gamepadButtonAssign);

		// (SGB)
		void setPlayerCount(int count);

	private:
		Memory* mem_;
		bool enabled_ = true;

		// P1 & 0x30
		uint8 selected_ = 0;

		// P1 & 0x0F
		uint8 dirState_ = 0;

		// P1 & 0x0F
		uint8 actState_ = 0;

		//uint8 joyp_ = 0;

		InputGroup inputRight_;
		InputGroup inputLeft_;
		InputGroup inputUp_;
		InputGroup inputDown_;
		InputGroup inputA_;
		InputGroup inputB_;
		InputGroup inputSelect_;
		InputGroup inputStart_;

		// (SGB) プレイヤー人数: 0 or 1 or 3 (MLT_REQ)
		uint8 playerCount_ = 0;

		// (SGB) Joypad ID
		uint8 joypadId_ = 0xf;
		bool readyForSwitchJoypad_ = false;

	};
}
