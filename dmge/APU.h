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
		int freq;

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

		void checkDAC();

		bool getDACEnable() const;

		bool getEnable() const;

		void setEnable(bool enable);

	private:
		Memory* mem_;

		Channels ch_;

		ChannelData data_{};

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
		bool sweepEnabled_ = 0;
		int shadowFreq_ = 0;

		// Length
		int lengthTimer_ = 0;
		//int chEnabledByLength_ = 1;

		// Noise
		uint16 lfsr_ = 0;

		int calcSweepFrequency_();
	};

	/*
	struct WaveChannel
	{
		// NR30
		bool enable;

		// NR31
		int lengthTimer;

		// NR32
		// bit 5-6 : 0=なし、1以上=その数だけ音量をシフト
		int outputLevel;

		// NR33 & NR34.0-2
		int freq;

		// NR34
		// bit 6
		bool enableLength;

		// NR34
		// bit 7
		bool trigger;
	};
	*/

	class APU
	{
	public:
		APU(Memory* mem);

		void update();

		void trigger(Channels ch);

	private:
		Memory* mem_;

		Audio audio_;

		Wave wave_;

		int samples_ = 0;

		Channel ch1_;
		Channel ch2_;
		Channel ch3_;
		Channel ch4_;

		// Frame Seq. Clock
		int fsClock_ = 0;

		// Count T-cycles
		int cycles_ = 0;

		uint8 prevDiv_ = 0;
	};
}
