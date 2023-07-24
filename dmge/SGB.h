#pragma once

namespace dmge
{
	class Joypad;

	class SGBCommand
	{
	public:
		SGBCommand(Joypad& joypad);

		// transferBits: (JOYP.4 | (JOYP.5 << 1))
		void send(uint8 transferBits);

		void dump() const;

	private:
		Joypad& joypad_;

		uint8 prevBits_ = 0b11;

		Array<uint8> received_;
		uint8 currentByte_ = 0;
		uint8 currentByteReceivedBits_ = 0;

		void processCommand_();
	};
}
