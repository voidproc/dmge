#pragma once

namespace dmge
{
	enum class SerialClockSource : uint8
	{
		External = 0,
		Internal = 1
	};

	enum class SerialClockSpeed : uint8
	{
		Normal = 0,
		Fast = 1
	};

	class Interrupt;

	class Serial
	{
	public:
		Serial(Interrupt& interrupt);

		void writeRegister(uint16 addr, uint8 value);

		uint8 readRegister(uint16 addr) const;

		bool transfering() const;

		void update();

	private:
		Interrupt& interrupt_;

		// SB (0xFF01)
		uint8 transferData_ = 0;

		// SB (0xff02)

		Optional<uint8> remainBits_{};
		SerialClockSpeed clockSpeed_{};
		SerialClockSource clockSource_{};
		
		uint8 clock_ = 0;
	};
}
