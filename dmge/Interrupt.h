#pragma once

namespace dmge
{
	// IME, IF (0xff0f), IE (0xffff)

	class Interrupt
	{
	public:

		// IME

		void reserveEnablingIME();

		void disableIME();

		void updateIME();

		bool ime() const;

		// IF & IE

		// 割り込み要求をする
		// IFの指定したビット (BitMask::InterruptFlagBit) をセットする
		void request(uint8 bit);

		// 割り込み要求をクリアする
		// IFの指定したビット (BitMask::InterruptFlagBit) をリセットする
		void clearRequest(uint8 bit);

		// 指定したビット (BitMask::InterruptFlagBit) の割り込みが有効(IE)かつ要求(IF)されているか
		bool requested(uint8 bit) const;

		// 何かしらの有効かつ要求された割り込みがあるか
		bool requested() const;

		// IOレジスタへの書き込み
		void writeRegister(uint16 addr, uint8 value);

		// IOレジスタからの読み込み
		uint8 readRegister(uint16 addr) const;

	private:
		// IME: Interrupt master enable flag

		bool ime_ = false;
		bool imeScheduled_ = false;

		// Interrupt registers

		uint8 if_ = 0;
		uint8 ie_ = 0;
	};
}
