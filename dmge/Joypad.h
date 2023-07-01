#pragma once

namespace dmge
{
	// 選択状態
	// JOYP:0xff00 の 4-5bit
	enum class SelectedButtons
	{
		Direction,
		Action,
	};

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
		uint8 readRegister() const;

		// 現在の選択状態（方向orアクション）に応じてIOレジスタを更新する
		void update();

		// ジョイパッド有効／無効を切り替える
		// 無効の場合、update()が呼ばれてもデバイスからボタンの状態を取得しない
		void setEnable(bool enable);

		// ボタン割り当てを設定
		void setButtonAssign(const GamepadButtonAssign& gamepadButtonAssign);

	private:
		Memory* mem_;
		SelectedButtons selected_ = SelectedButtons::Direction;
		bool enabled_ = true;
		uint8 joyp_ = 0;

		InputGroup inputRight_;
		InputGroup inputLeft_;
		InputGroup inputUp_;
		InputGroup inputDown_;
		InputGroup inputA_;
		InputGroup inputB_;
		InputGroup inputSelect_;
		InputGroup inputStart_;
	};
}
