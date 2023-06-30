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

		add1Second_();

		regInternal_.s &= MaskS;
		regInternal_.m &= MaskM;
		regInternal_.h &= MaskH;
		regInternal_.d &= MaskD;
	}

	RTCSaveData RTC::getSaveData()
	{
		RTCSaveData savedata;

		savedata.seconds = regInternal_.s;
		savedata.minutes = regInternal_.m;
		savedata.hours = regInternal_.h;
		savedata.days = (regInternal_.d & 0xff) | ((static_cast<uint64>(regInternal_.d) & 0xff00) << (8 * 3));

		savedata.secondsLatched = regLatched_.s;
		savedata.minutesLatched = regLatched_.m;
		savedata.hoursLatched = regLatched_.h;
		savedata.daysLatched = (regLatched_.d & 0xff) | ((regLatched_.d & 0xff00) << (8 * 3));

		savedata.timestamp = Time::GetSecSinceEpoch();

		return savedata;
	}

	void RTC::loadSaveData(const RTCSaveData& rtcSaveData)
	{
		const uint64 elapsedSec = Time::GetSecSinceEpoch() - rtcSaveData.timestamp;

		const int s = rtcSaveData.seconds + elapsedSec;
		regInternal_.s = static_cast<uint8>(s % 60);

		const int m = rtcSaveData.minutes + (s / 60);
		regInternal_.m = static_cast<uint8>(m % 60);

		const int h = rtcSaveData.hours + (m / 60);
		regInternal_.h = static_cast<uint8>(h % 24);

		const uint16 d67 = (rtcSaveData.days >> (8 * 3)) & 0b11000000'00000000;
		const uint16 d = ((rtcSaveData.days & 0xff) | ((rtcSaveData.days >> (8 * 3)) & 0x100)) + (h / 24);  //ここでキャリーを考慮すべきかわからない
		regInternal_.d = d67 | (d & 0x1ff);

		regLatched_.s = static_cast<uint8>(rtcSaveData.secondsLatched);
		regLatched_.m = static_cast<uint8>(rtcSaveData.minutesLatched);
		regLatched_.h = static_cast<uint8>(rtcSaveData.hoursLatched);
		regLatched_.d = (rtcSaveData.daysLatched & 0xff) | ((rtcSaveData.daysLatched >> (8 * 3)) & 0xff00);
	}

	void RTC::dump()
	{
		Console.writeln(U"RTC Internal: s={:02x} m={:02x} h={:02x} d={:04x}"_fmt(regInternal_.s, regInternal_.m, regInternal_.h, regInternal_.d));
		Console.writeln(U"RTC Latched : s={:02x} m={:02x} h={:02x} d={:04x}"_fmt(regLatched_.s, regLatched_.m, regLatched_.h, regLatched_.d));
	}

	void RTC::latch_()
	{
		regLatched_ = regInternal_;
	}

	void RTC::add1Second_()
	{
		if (++regInternal_.s != 60) return;
		regInternal_.s = 0;
		if (++regInternal_.m != 60) return;
		regInternal_.m = 0;
		if (++regInternal_.h != 24) return;
		regInternal_.h = 0;
		if (++regInternal_.d != 512) return;
		regInternal_.d = 0;
		carry_ = true;
	}
}
