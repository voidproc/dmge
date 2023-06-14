#pragma once

#include "Address.h"

namespace dmge
{
	class Memory;


	//[DEBUG]
	inline uint64 g_clock = 0;


	// VolumeEnvelope

	class VolumeEnvelope
	{
	public:
		void step();

		void trigger();

		void set(uint8 NRx2);

		int volume() const;

		int initialVolume() const;

	private:
		int initialVolume_ = 0;
		int direction_ = 0;
		int period_ = 0;
		int currentVolume_ = 0;
		int periodTimer_ = 0;
	};


	// FrequencySweep

	class Channel;

	class FrequencySweep
	{
	public:
		FrequencySweep(Channel* channel);

		int step(int freq);

		void trigger(int freq);

		void set(uint8 NRx0);

	private:
		Channel* channel_;

		int period_ = 0;
		int direction_ = 0;
		int shift_ = 0;
		int sweepTimer_ = 0;
		bool enabled_ = false;
		int shadowFreq_ = 0;

		int calcSweepFrequency_();
	};


	// LengthCounter

	class LengthCounter
	{
	public:
		LengthCounter(Channel* channel);

		void step();

		void trigger(int newLengthTimer);

		void setEnable(bool enable);

		bool getEnable() const;

		void setLengthTimer(int value);

		void setExtraLengthClockCond(bool cond);

	private:
		Channel* channel_;

		bool enabled_ = false;
		int lengthTimer_ = 0;
		bool extraLengthClockCond_ = false;
	};


	// Channel

	class Channel
	{
	public:
		bool getEnable() const;

		void setEnable(bool enable);

	private:
		// Enabled (NR52)
		bool enabled_ = false;
	};


	// SquareChannel

	class SquareChannel : public Channel
	{
	public:
		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Square duty

		void setDuty(uint8 NRx1);

		// Envelope

		void stepEnvelope();

		void setEnvelope(uint8 NRx2);

		// Sweep

		void stepSweep();

		void setSweep(uint8 NRx0);

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Frequency

		void setFrequencyLow(uint8 freqLow);

		void setFrequencyHigh(uint8 freqHigh);

		int getFrequency();

	private:
		VolumeEnvelope envelope_{};

		FrequencySweep sweep_{ this };

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Duty ratio
		int duty_ = 0;

		// Duty position
		int dutyPos_ = 0;

		// Waveform frequency
		int freq_ = 0;
	};


	// WaveChannel

	class WaveChannel : public Channel
	{
	public:
		WaveChannel(Memory* mem);

		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Frequency

		void setFrequencyLow(uint8 freqLow);

		void setFrequencyHigh(uint8 freqHigh);

		int getFrequency();

		// Wave volume

		void setWaveOutputLevel(uint8 NRx2);

	private:
		Memory* mem_;

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Waveform frequency
		int freq_ = 0;

		// Wave RAM offset (0-31)
		int waveRAMOffset_ = 0;

		// Wave volume
		int waveOutputLevel_ = 0;
	};


	// NoiseChannel

	class NoiseChannel : public Channel
	{
	public:
		// Frequency Timer を進める
		void step();

		void trigger();

		int amplitude() const;

		// Envelope

		void stepEnvelope();

		void setEnvelope(uint8 NRx2);

		// Length

		void stepLength();

		void setEnableLength(bool enable);

		void setLengthTimer(uint8 NRx1);

		void setExtraLengthClockCondition(bool value);

		// Noise

		void setRandomness(uint8 NRx3);

	private:
		VolumeEnvelope envelope_{};

		LengthCounter length_{ this };

		// Frequency timer
		int freqTimer_ = 0;

		// Noise randomness

		int divisorShift_ = 0;
		int counterWidth_ = 0;
		int divisor_ = 0;
		uint16 lfsr_ = 0;

	};
}
