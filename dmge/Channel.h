#pragma once

namespace dmge
{
	class Memory;

	struct ChannelData
	{
		bool enable;
		int outputLevel;

		int sweepPeriod;
		int sweepDir;
		int sweepShift;

		int duty;
		int lengthTimer;
		int envVol;
		int envDir;
		int envPeriod;
		bool trigger;
		bool enableLength;

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

		void setTriggerFlag();

		bool onTrigger() const;

		int amplitude() const;

		bool getDACEnable() const;

		bool getEnable() const;

		void setEnable(bool enable);

		void setLengthTimer(uint8 reg);

		void setFrequency(int freq);

		int getFrequency();

	private:
		Memory* mem_;

		Channels ch_;

		ChannelData data_{};

		// Enabled (NR52)
		bool enabled_ = false;

		// Trigger Flag
		bool trigger_ = false;

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
		int lengthTimer_ = 0;

		// Noise
		uint16 lfsr_ = 0;

		int calcSweepFrequency_();
	};

}
