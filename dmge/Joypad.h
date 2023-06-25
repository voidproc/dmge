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

	class Joypad
	{
	public:
		Joypad(Memory* mem);

		// 現在の選択状態（方向orアクション）に応じてIOレジスタ(JOYP:0xff00)を更新する
		void update();

		// IOレジスタ(JOYP:0xff00)にvalueが書き込まれる前に呼び出される
		// 選択状態（方向orアクション）とIOレジスタ(JOYP:0xff00)を更新する
		void update(uint8 value);

		// ジョイパッド有効／無効を切り替える
		// 無効の場合、update()が呼ばれてもデバイスからボタンの状態を取得しない
		void setEnable(bool enable);

	private:
		Memory* mem_;
		SelectedButtons selected_ = SelectedButtons::Direction;
		bool enabled_ = true;
	};
}
