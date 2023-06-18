#pragma once

namespace dmge
{
	enum class RTCRegisters : uint8
	{
		S = 0x8,
		M = 0x9,
		H = 0xa,
		DL = 0xb,
		DH = 0xc,
	};

	struct RTCRegister
	{
		uint8 s = 0;
		uint8 m = 0;
		uint8 h = 0;
		uint16 d = 0;
	};

	class RTC
	{
	public:
		RTC();

		void setEnable(bool enable);

		bool enabled() const;

		// RTCレジスタの選択
		// value: 0x8～0xc
		void select(uint8 value);

		void unselect();

		bool selected() const;

		void writeLatchClock(uint8 value);

		void writeRegister(uint8 value);

		uint8 readRegister() const;

		void update(int cycles);

	private:
		bool enabled_ = false;

		Optional<RTCRegisters> selected_{};

		bool preparedLatch_ = false;

		RTCRegister regInternal_;

		RTCRegister regLatched_;

		bool halt_ = false;
		bool carry_ = false;

		time_t timeRTCEnabled_{};

		int cycles_ = 0;

		void latch_();
	};
}
