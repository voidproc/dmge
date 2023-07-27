#include "stdafx.h"
#include "SGB.h"
#include "Joypad.h"
#include "LCD.h"
#include "PPU.h"
#include <magic_enum/magic_enum.hpp>

namespace dmge
{
	namespace SGB
	{
		Command::Command(Joypad& joypad, LCD& lcd, PPU& ppu)
			: joypad_{ joypad }, lcd_{ lcd }, ppu_{ ppu }
		{
			received_.reserve(128);
		}

		void Command::send(uint8 transferBits)
		{
			if (prevBits_ == 0b11 && transferBits != 0b11)
			{
				if (transferBits == 0)
				{
					if (state_ == TransferState::Stop)
					{
						// Reset

						if (packetLength_ <= 0)
						{
							received_.clear();
							receivedSizePartial_ = 0;
							packetLength_ = 0;
						}

						currentByte_ = 0;
						currentByteReceivedBits_ = 0;
						state_ = TransferState::Transfering;
					}
				}
				else if (transferBits == 0b10)
				{
					// 0

					if (state_ == TransferState::Transfering)
					{
						if (receivedSizePartial_ == 16)
						{
							// Stop

							// パケットの長さを調べる
							// 長さが 2 以上なら今のコマンドを継続して受け取る必要がある
							if (packetLength_ == 0)
							{
								packetLength_ = received_[0] & 0b111;
							}

							if (--packetLength_ <= 0)
							{
								dump();

								processCommand_();
								received_.clear();
							}

							receivedSizePartial_ = 0;
							state_ = TransferState::Stop;
						}
						else
						{
							++currentByteReceivedBits_;
						}
					}
				}
				else if (transferBits == 0b01)
				{
					// 1

					if (state_ == TransferState::Transfering)
					{
						currentByte_ |= 1 << currentByteReceivedBits_;
						++currentByteReceivedBits_;
					}
				}

				if (currentByteReceivedBits_ >= 8)
				{
					received_.push_back(currentByte_);
					++receivedSizePartial_;
					currentByte_ = 0;
					currentByteReceivedBits_ = 0;
				}
			}

			prevBits_ = transferBits;
		}

		void Command::dump() const
		{
			if (received_.isEmpty()) return;

			const uint8 command = received_[0] >> 3;
			const auto commandName = magic_enum::enum_name((Functions)command);
			Console.write(U"SGB Command={:02X} {:10s} Length={} "_fmt(command, Unicode::WidenAscii(commandName), received_[0] & 7));

			for (const auto byte : received_)
			{
				Console.write(U"{:02X} "_fmt(byte));
			}

			Console.writeln();
		}

		void Command::processCommand_()
		{
			if (received_.isEmpty()) return;

			const Functions func = static_cast<Functions>(received_[0] >> 3);

			// PALXX : 色番号 0 をパレット 0～3 に適用

			switch (func)
			{
			case Functions::PAL01:
			case Functions::PAL23:
			case Functions::PAL03:
			case Functions::PAL12:
				const uint16 color0 = received_[1] | (received_[2] << 8);
				for (int i = 0; i < 4; i++)
				{
					lcd_.setSGBPalette(i, 0, color0);
				}
			}

			switch (func)
			{
			case Functions::PAL01:
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(0, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(1, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;

			case Functions::PAL23:
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(2, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(3, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;

			case Functions::PAL03:
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(0, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(3, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;

			case Functions::PAL12:
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(1, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(2, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;

			case Functions::PAL_SET:
			{
				const uint16 palette0 = received_[1] | (received_[2] << 8);
				const uint16 palette1 = received_[3] | (received_[4] << 8);
				const uint16 palette2 = received_[5] | (received_[6] << 8);
				const uint16 palette3 = received_[7] | (received_[8] << 8);
				lcd_.setSGBPalette(palette0, palette1, palette2, palette3);

				// アトリビュートファイルの指定 (received_[9])
				//...

				break;
			}

			case Functions::PAL_TRN:
				lcd_.transferSystemColorPalette();
				break;

			case Functions::ATTR_TRN:
				ppu_.transferAttributeFile();
				break;

			case Functions::MLT_REQ:
				joypad_.setPlayerCount(received_[1] & 3);
				break;
			}
		}
	}
}
