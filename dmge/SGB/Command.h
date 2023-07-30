#pragma once

#include "Commands.h"

namespace dmge
{
	class Joypad;
	class LCD;
	class PPU;

	namespace SGB
	{
		// SGBパケットの転送状況
		enum class PacketTransferState
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

			PacketTransferState state_ = PacketTransferState::Stop;

			void processCommand_();
		};
	}
}
