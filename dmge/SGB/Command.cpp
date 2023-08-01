#include "Command.h"
#include "../Joypad.h"
#include "../LCD.h"
#include "../PPU.h"
#include "../DebugPrint.h"
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
					if (state_ == PacketTransferState::Stop)
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
						state_ = PacketTransferState::Transfering;
					}
				}
				else if (transferBits == 0b10)
				{
					// 0

					if (state_ == PacketTransferState::Transfering)
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
							state_ = PacketTransferState::Stop;
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

					if (state_ == PacketTransferState::Transfering)
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
			const auto commandName = magic_enum::enum_name(ToEnum<Commands>(command));
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

			const auto func = ToEnum<Commands>(received_[0] >> 3);

			// PALXX : 色番号 0 をパレット 0～3 に適用

			switch (func)
			{
			case Commands::PAL01:
			case Commands::PAL23:
			case Commands::PAL03:
			case Commands::PAL12:
				const uint16 color0 = received_[1] | (received_[2] << 8);
				for (int i = 0; i < 4; i++)
				{
					lcd_.setSGBPalette(i, 0, color0);
				}
			}

			switch (func)
			{
			case Commands::PAL01:
			{
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(0, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(1, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;
			}

			case Commands::PAL23:
			{
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(2, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(3, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;
			}

			case Commands::PAL03:
			{
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(0, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(3, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;
			}

			case Commands::PAL12:
			{
				for (int i = 0; i < 3; i++)
				{
					lcd_.setSGBPalette(1, i + 1, received_[3 + i * 2] | (received_[3 + i * 2 + 1] << 8));
					lcd_.setSGBPalette(2, i + 1, received_[9 + i * 2] | (received_[9 + i * 2 + 1] << 8));
				}
				break;
			}

			case Commands::PAL_SET:
			{
				const uint16 palette0 = received_[1] | (received_[2] << 8);
				const uint16 palette1 = received_[3] | (received_[4] << 8);
				const uint16 palette2 = received_[5] | (received_[6] << 8);
				const uint16 palette3 = received_[7] | (received_[8] << 8);
				lcd_.setSGBPalette(palette0, palette1, palette2, palette3);

				// アトリビュートファイルの指定
				if (received_[9] & 0x80)
				{
					const int atf = Min<uint8>(received_[9] & 0x3f, 0x2c);
					ppu_.setAttribute(atf);
				}

				break;
			}

			case Commands::PAL_TRN:
			{
				lcd_.transferSystemColorPalette();
				break;
			}

			case Commands::PAL_PRI:
				DebugPrint::Writeln(U"PAL_PRI: Not implement");
				break;

			case Commands::ATTR_BLK:
			{
				const uint8 groupCount = Min<uint8>(received_[1] & 0x1f, 0x12);

				for (int group = 0; group < groupCount; ++group)
				{
					const auto offset = group * 6;
					const uint8 controlCode = received_[2 + offset] & 7;
					const uint8 paletteInsideArea = received_[3 + offset] & 3;
					const uint8 paletteOnBorder = (received_[3 + offset] >> 2) & 3;
					const uint8 paletteOutsideArea = (received_[3 + offset] >> 4) & 3;
					const uint8 left = received_[4 + offset] & 0x1f;
					const uint8 top = received_[5 + offset] & 0x1f;
					const uint8 right = received_[6 + offset] & 0x1f;
					const uint8 bottom = received_[7 + offset] & 0x1f;

					// Outside the square

					if (controlCode & 0b100)
					{
						for (int y = 0; y < 144 / 8; ++y)
						{
							for (int x = 0; x < 160 / 8; ++x)
							{
								if ((x < left || x > right || y < top || y > bottom))
								{
									ppu_.setAttribute(x, y, paletteOutsideArea);
								}
							}
						}
					}

					// Inside the square

					if (controlCode & 0b001)
					{
						for (int y = top; y <= bottom; ++y)
						{
							for (int x = left; x <= right; ++x)
							{
								ppu_.setAttribute(x, y, paletteInsideArea);
							}
						}
					}

					// On border

					if (controlCode & 0b010)
					{
						for (int x = left; x <= right; ++x)
						{
							ppu_.setAttribute(x, top, paletteOnBorder);
							ppu_.setAttribute(x, bottom, paletteOnBorder);
						}

						for (int y = top + 1; y <= bottom - 1; ++y)
						{
							ppu_.setAttribute(left, y, paletteOnBorder);
							ppu_.setAttribute(right, y, paletteOnBorder);
						}
					}
				}

				break;
			}

			case Commands::ATTR_LIN:
			{
				const int groupCount = Min<uint8>(received_[1], 0x6e);

				for (int group = 0; group < groupCount; ++group)
				{
					const uint8 palette = (received_[2 + group] >> 5) & 3;
					const uint8 lineNumber = received_[2 + group] & 0x1f;

					if ((received_[2 + group] & 0x80) == 0x80)
					{
						// V coord (horizontal line)
						for (int x = 0; x < 160 / 8; ++x)
						{
							ppu_.setAttribute(x, lineNumber, palette);
						}
					}
					else
					{
						// H coord (vertical line)
						for (int y = 0; y < 144 / 8; ++y)
						{
							ppu_.setAttribute(lineNumber, y, palette);
						}
					}
				}
				break;
			}

			case Commands::ATTR_DIV:
			{
				const uint8 paletteBottomOrRight = received_[1] & 3;
				const uint8 paletteTopOrLeft = (received_[1] >> 2) & 3;
				const uint8 paletteOnBorder = (received_[1] >> 4) & 3;
				const bool divideHorizontal = received_[1] & 0x40;
				const uint8 dividePos = received_[2] & 0x1f;

				for (int y = 0; y < 144 / 8; ++y)
				{
					for (int x = 0; x < 160 / 8; ++x)
					{
						const uint8 pos = divideHorizontal ? y : x;

						if (pos < dividePos)
						{
							ppu_.setAttribute(x, y, paletteTopOrLeft);
						}
						else if (pos > dividePos)
						{
							ppu_.setAttribute(x, y, paletteBottomOrRight);
						}
						else if (pos == dividePos)
						{
							ppu_.setAttribute(x, y, paletteOnBorder);
						}
					}
				}

				break;
			}

			case Commands::ATTR_CHR:
			{
				const uint8 startX = received_[1] & 0x1f;
				const uint8 startY = received_[2] & 0x1f;
				const int count = Min((received_[3] | (received_[4] << 8)) & 0x1ff, 360);

				int x = startX;
				int y = startY;

				for (int i = 0; i < count; i++)
				{
					const int index = 6 + i / 4;
					const int shiftBits = (3 - i % 4) * 2;
					const uint8 palette = (received_[index] >> shiftBits) & 3;

					ppu_.setAttribute(x, y, palette);

					if (received_[5] & 1)
					{
						++y;

						if (y == 144 / 8)
						{
							y = 0;
							++x;
						}

						if (x == 160 / 8)
						{
							x = 0;
						}
					}
					else
					{
						++x;

						if (x == 160 / 8)
						{
							x = 0;
							++y;
						}

						if (y == 144 / 8)
						{
							y = 0;
						}
					}
				}
				break;
			}

			case Commands::ATTR_TRN:
			{
				ppu_.transferAttributeFiles();
				break;
			}

			case Commands::ATTR_SET:
			{
				const int atf = Min<uint8>(received_[1] & 0x3f, 0x2c);
				ppu_.setAttribute(atf);
				break;
			}

			case Commands::SOUND:
				DebugPrint::Writeln(U"SOUND: Not implement");
				break;

			case Commands::SOU_TRN:
				DebugPrint::Writeln(U"SOU_TRN: Not implement");
				break;

			case Commands::MASK_EN:
				DebugPrint::Writeln(U"MASK_EN: Not implement");
				break;

			case Commands::ATRC_EN:
				DebugPrint::Writeln(U"ATRC_EN: Not implement");
				break;

			case Commands::TEST_EN:
				DebugPrint::Writeln(U"TEST_EN: Not implement");
				break;

			case Commands::ICON_EN:
				DebugPrint::Writeln(U"ICON_EN: Not implement");
				break;

			case Commands::DATA_SND:
				DebugPrint::Writeln(U"DATA_SND: Not implement");
				break;

			case Commands::DATA_TRN:
				DebugPrint::Writeln(U"DATA_TRN: Not implement");
				break;

			case Commands::JUMP:
				DebugPrint::Writeln(U"JUMP: Not implement");
				break;

			case Commands::MLT_REQ:
			{
				joypad_.setPlayerCount(received_[1] & 3);
				break;
			}

			case Commands::CHR_TRN:
				DebugPrint::Writeln(U"CHR_TRN: Not implement");
				break;

			case Commands::PCT_TRN:
				DebugPrint::Writeln(U"PCT_TRN: Not implement");
				break;

			case Commands::OBJ_TRN:
				DebugPrint::Writeln(U"OBJ_TRN: Not implement");
				break;

			}
		}
	}
}
