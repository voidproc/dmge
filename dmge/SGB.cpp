#include "stdafx.h"
#include "SGB.h"
#include "Joypad.h"
#include <magic_enum/magic_enum.hpp>

namespace dmge
{
	namespace SGB
	{
		PacketTransfer::PacketTransfer(Joypad& joypad)
			: joypad_{ joypad }
		{
			received_.reserve(128);
		}

		void PacketTransfer::send(uint8 transferBits)
		{
			if (prevBits_ == 0b11 && transferBits != 0b11)
			{
				if (transferBits == 0)
				{
					if (state_ == TransferState::Stop)
					{
						// Reset

						received_.clear();
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
						if (received_.size() == 16)
						{
							// Stop

							dump();

							processCommand_();

							received_.clear();
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
					currentByte_ = 0;
					currentByteReceivedBits_ = 0;
				}
			}

			prevBits_ = transferBits;
		}

		void PacketTransfer::dump() const
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

		void PacketTransfer::processCommand_()
		{
			if (received_.isEmpty()) return;

			const uint8 command = received_[0] >> 3;

			if (command == 0x11)
			{
				// MLT_REQ

				joypad_.setPlayerCount(received_[1] & 3);
			}
		}
	}
}
