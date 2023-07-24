#include "stdafx.h"
#include "SGB.h"
#include "Joypad.h"

namespace dmge
{
	SGBCommand::SGBCommand(Joypad& joypad)
		: joypad_{ joypad }
	{
		received_.reserve(128);
	}

	void SGBCommand::send(uint8 transferBits)
	{
		if (prevBits_ == 0b11)
		{
			if (transferBits == 0)
			{
				// Reset

				received_.clear();
				currentByte_ = 0;
				currentByteReceivedBits_ = 0;
			}
			else if (transferBits == 0b10)
			{
				// 0

				if (received_.size() == 16)
				{
					// Stop

					dump();

					processCommand_();

					received_.clear();
				}
				else
				{
					++currentByteReceivedBits_;
				}
			}
			else if (transferBits == 0b01)
			{
				// 1

				currentByte_ |= 1 << currentByteReceivedBits_;
				++currentByteReceivedBits_;
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

	void SGBCommand::dump() const
	{
		if (received_.isEmpty()) return;

		Console.write(U"SGB Command={:02X} Length={} "_fmt(received_[0] >> 3, received_[0] & 7));

		for (const auto byte : received_)
		{
			Console.write(U"{:02X} "_fmt(byte));
		}

		Console.writeln();
	}

	void SGBCommand::processCommand_()
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
