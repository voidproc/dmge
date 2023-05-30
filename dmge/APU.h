#pragma once

#include "Channel.h"


namespace dmge
{
	class Memory;


	class APUStream : public IAudioStream
	{
	public:
		APUStream();

		virtual ~APUStream();

		void pushSample(float left, float right);

		int bufferSize() const;

	private:
		virtual void getAudio(float* left, float* right, size_t samplesToWrite) override;

		virtual bool hasEnded() override;

		virtual void rewind() override;

	private:
		Wave wave_;

		int posPushed_ = 0;

		int posRead_ = 0;

		int bufferSize_ = 0;

	};


	class APU
	{
	public:
		APU(Memory* mem);

		void update();

		void setFrequency(Channels ch, int freq);
		void trigger(Channels ch);
		void setLengthTimer(Channels ch, uint8 reg);

	private:
		Memory* mem_;

		std::shared_ptr<APUStream> apuStream_;

		Audio audio_;

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
