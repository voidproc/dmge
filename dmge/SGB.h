#pragma once

namespace dmge
{
	class Joypad;
	class LCD;
	class PPU;

	namespace SGB
	{
		// SGB System Commands
		enum class Commands : uint8
		{
			PAL01 = 0x00,
			PAL23 = 0x01,
			PAL03 = 0x02,
			PAL12 = 0x03,
			PAL_SET = 0x0a,
			PAL_TRN = 0x0b,
			PAL_PRI = 0x19,

			ATTR_BLK = 0x04,
			ATTR_LIN = 0x05,
			ATTR_DIV = 0x06,
			ATTR_CHR = 0x07,
			ATTR_TRN = 0x15,
			ATTR_SET = 0x16,

			SOUND = 0x08,
			SOU_TRN = 0x09,

			MASK_EN = 0x17,
			ATRC_EN = 0x0c,
			TEST_EN = 0x0d,
			ICON_EN = 0x0e,
			DATA_SND = 0x0f,
			DATA_TRN = 0x10,
			JUMP = 0x12,

			MLT_REQ = 0x11,

			CHR_TRN = 0x13,
			PCT_TRN = 0x14,
			OBJ_TRN = 0x18,
		};

		// SGBパケットの転送状況
		enum class TransferState
		{
			// パケットをまだ受信していない or Stopビットを受信した
			Stop,

			// Resetパケットを受信し転送開始している
			Transfering,
		};

		class Command
		{
		public:
			Command(Joypad& joypad, LCD& lcd, PPU& ppu);

			/// @brief P1 レジスタに書き込まれる Bit4, Bit5 を SGB コマンドパケットの送信と解釈し処理する
			/// @param transferBits (P1.4 | (P1.5 << 1))
			void send(uint8 transferBits);

			// [DEBUG]
			void dump() const;

		private:
			Joypad& joypad_;
			LCD& lcd_;
			PPU& ppu_;

			// 前回の P1.4, P1.5
			uint8 prevBits_ = 0b11;

			// 受信した SGB パケット
			Array<uint8> received_;

			// 受信した SGB パケット (16bytes ごとにリセットされる)
			int receivedSizePartial_ = 0;

			// 現在受信中のバイト
			uint8 currentByte_ = 0;

			// 現在受信中のビット位置 (0-7)
			uint8 currentByteReceivedBits_ = 0;

			// 未受信のパケット数
			int packetLength_ = 0;

			TransferState state_ = TransferState::Stop;

			void processCommand_();
		};
	}
}
