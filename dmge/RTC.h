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

#pragma pack(4)
	struct RTCSaveData
	{
		uint32 seconds;
		uint32 minutes;
		uint32 hours;
		uint64 days;

		uint32 secondsLatched;
		uint32 minutesLatched;
		uint32 hoursLatched;
		uint64 daysLatched;

		uint64 timestamp;
	};
#pragma pack()

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

		RTCSaveData getSaveData();

		void loadSaveData(const RTCSaveData& rtcSaveData);

		// [DEBUG]
		void dump();

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

		void add1Second_();
	};
}
