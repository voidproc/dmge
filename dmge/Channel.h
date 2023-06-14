#pragma once

#include "Address.h"

namespace dmge
{
	class Memory;

	struct ChannelData
	{
		// NR10

		int sweepPeriod;
		int sweepDir;
		int sweepShift;

		// NR11, NR21

		int duty;

		// NR12, NR22, NR42

		int envVol;
		int envDir;
		int envPeriod;

		// NR32

		int outputLevel;

		// NR43

		int divisorShift;
		int counterWidth;
		int divisor;
	};


	enum class Channels
	{
		Ch1,
		Ch2,
		Ch3,
		Ch4,
	};

	//[DEBUG]
	inline uint64 g_clock = 0;


	class Channel
	{
	public:
		Channel(Memory* mem, Channels ch);

		void fetch();

		void updateFrequencyTimer();

		void doTrigger();

		void doEnvelope();

		void doSweep();

		void doLength();

		int amplitude() const;

		bool getEnable() const;

		void setEnable(bool enable);

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 reg);

		void setExtraLengthClockCondition(bool value);

		void setFrequencyLow(uint8 freqLow);
		void setFrequencyHigh(uint8 freqHigh);

		int getFrequency();

	private:
		Memory* mem_;

		Channels ch_;

		ChannelData data_{};

		// Enabled (NR52)
		bool enabled_ = false;

		// Frequency timer
		int freqTimer_ = 0;

		// Duty position
		int dutyPos_ = 0;

		// Wave RAM offset (0-31)
		int waveRAMOffset_ = 0;

		// Waveform frequency
		int freq_ = 0;

		// Envelope
		int currentVolume_ = 0;
		int periodTimer_ = 0;

		// Sweep
		int sweepTimer_ = 0;
		bool sweepEnabled_ = false;
		int shadowFreq_ = 0;

		// Length
		bool enableLength_ = false;
		int lengthTimer_ = 0;
		bool extraLengthClockCond_ = false;

		// Noise
		uint16 lfsr_ = 0;

		int calcSweepFrequency_();
	};

}
