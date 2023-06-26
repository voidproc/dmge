#include "Interrupt.h"
#include "Address.h"

namespace dmge
{
	void Interrupt::reserveEnablingIME()
	{
		imeScheduled_ = true;
	}

	void Interrupt::disableIME()
	{
		ime_ = false;
	}

	void Interrupt::updateIME()
	{
		if (imeScheduled_)
		{
			imeScheduled_ = false;
			ime_ = true;
		}
	}

	bool Interrupt::ime() const
	{
		return ime_;
	}

	void Interrupt::request(uint8 bit)
	{
		if_ |= (1 << bit);
	}

	void Interrupt::clearRequest(uint8 bit)
	{
		if_ &= ~(1 << bit);
	}

	bool Interrupt::requested(uint8 bit) const
	{
		return (if_ & ie_) & (1 << bit);
	}

	bool Interrupt::requested() const
	{
		return (if_ & ie_) != 0;
	}

	void Interrupt::writeRegister(uint16 addr, uint8 value)
	{
		if (addr == Address::IF)
		{
			if_ = value;
		}
		else if (addr == Address::IE)
		{
			ie_ = value;
		}
	}

	uint8 Interrupt::readRegister(uint16 addr) const
	{
		if (addr == Address::IF)
		{
			return if_ | 0xe0;
		}
		else if (addr == Address::IE)
		{
			return ie_;
		}

		return 0;
	}
}
