#pragma once

namespace dmge
{
	class Memory;

	struct Channel
	{
		int duty;
		int lengthTimer;
		int envVol;
		int envDir;
		int envSweepPace;
		bool trigger;
		bool enableLength;
		int freq;
	};

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


	class APU
	{
	public:
		APU(Memory* mem);

		void update();

	private:
		Memory* mem_;

		Audio audio_[2];
		int audioIndex_ = 0;

		Wave wave_;

		int clock_ = 0;
		uint64 clock2_ = 0;

		int samples_ = 0;
		double lastSample = 0;
		int waveOffset_ = 0;

		Channel prevCh1_{};
		Channel prevCh2_{};
		WaveChannel prevCh3_{};

		bool ch1On_ = false;
		bool ch2On_ = false;
		bool ch3On_ = false;
	};
}
