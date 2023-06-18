#include "RTC.h"
#include "Timing.h"

namespace dmge
{
	constexpr uint8 MaskS = 0b00111111;
	constexpr uint8 MaskM = 0b00111111;
	constexpr uint8 MaskH = 0b00011111;
	constexpr uint8 MaskDL = 0b11111111;
	constexpr uint8 MaskDH = 0b11000001;
	constexpr uint16 MaskD = 0b111111111;

	RTC::RTC()
	{
		regLatched_ = RTCRegister{ 0xff, 0xff, 0xff, 0xffff };
	}

	void RTC::setEnable(bool enable)
	{
		enabled_ = enable;

		//if (enable)
		//{
			//timeRTCEnabled_ = time(0);
			//tm* t = localtime(&timeRTCEnabled_);
		//}
	}

	bool RTC::enabled() const
	{
		return enabled_;
	}

	void RTC::select(uint8 value)
	{
		switch (value)
		{
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
		case 0xc:
			selected_ = static_cast<RTCRegisters>(value);
			break;

		default:
			selected_ = none;
		}
	}

	void RTC::unselect()
	{
		selected_ = none;
	}

	bool RTC::selected() const
	{
		return selected_.has_value();
	}

	void RTC::writeLatchClock(uint8 value)
	{
		if (value == 0)
		{
			preparedLatch_ = true;
			return;
		}

		if (preparedLatch_)
		{
			if (value == 1)
			{
				latch_();
			}

			preparedLatch_ = false;
		}
	}

	void RTC::writeRegister(uint8 value)
	{
		if (not enabled_ || not selected_) return;

		switch (*selected_)
		{
		case RTCRegisters::S:  regInternal_.s = value & MaskS; cycles_ = 0; break;
		case RTCRegisters::M:  regInternal_.m = value & MaskM; break;
		case RTCRegisters::H:  regInternal_.h = value & MaskH; break;
		case RTCRegisters::DL: regInternal_.d = (regInternal_.d & 0xff00) | value; break;

		case RTCRegisters::DH:
			regInternal_.d = (regInternal_.d & 0x00ff) | ((value & 1) << 8);
			halt_ = (value >> 6) & 1;
			carry_ = (value >> 7) & 1;
			break;
		}
	}

	uint8 RTC::readRegister() const
	{
		if (not enabled_) return 0xff;

		switch (*selected_)
		{
		case RTCRegisters::S:  return regLatched_.s; break;
		case RTCRegisters::M:  return regLatched_.m; break;
		case RTCRegisters::H:  return regLatched_.h; break;
		case RTCRegisters::DL: return regLatched_.d & 0xff; break;
		case RTCRegisters::DH: return (regLatched_.d >> 8) | (carry_ << 7) | (halt_ << 6); break;
		}

		return 0xff;
	}

	void RTC::update(int cycles)
	{
		if (halt_) return;

		cycles_ += cycles;

		if (cycles_ < ClockFrequency) return;

		cycles_ -= ClockFrequency;

		if (++regInternal_.s != 60) goto mask;
		regInternal_.s = 0;
		if (++regInternal_.m != 60) goto mask;
		regInternal_.m = 0;
		if (++regInternal_.h != 24) goto mask;
		regInternal_.h = 0;
		if (++regInternal_.d != 512) goto mask;
		regInternal_.d = 0;
		carry_ = true;

	mask:
		regInternal_.s &= MaskS;
		regInternal_.m &= MaskM;
		regInternal_.h &= MaskH;
		regInternal_.d &= MaskD;
	}

	void RTC::latch_()
	{
		regLatched_ = regInternal_;
	}
}
