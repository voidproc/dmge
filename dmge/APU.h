﻿#pragma once

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

		int bufferRemain() const;

		int bufferMaxSize() const;

		// [DEBUG]
		std::pair<int, int> getSamplePos();

	private:
		virtual void getAudio(float* left, float* right, size_t samplesToWrite) override;

		virtual bool hasEnded() override;

		virtual void rewind() override;

	private:
		Wave wave_;

		int posWrite_ = 0;

		int posRead_ = 0;

		int bufferSize_ = 0;

	};


	class APU
	{
	public:
		APU(Memory* mem);

		// サウンド処理を1クロック分実行し、
		// サンプリングレートの周期にある場合はオーディオストリームにサンプルを書き込む
		void run();

		// オーディオストリームが十分にバッファリングされている場合は再生を開始し、
		// そうでない場合は再生を一時停止する
		void updatePlaybackState();

		// オーディオストリームの再生を一時停止する
		void pause();

		// チャンネル操作

		void setFrequency(Channels ch, int freq);
		void setEnvelopeAndDAC(Channels ch, uint8 reg);
		void trigger(Channels ch);
		void setLengthTimer(Channels ch, uint8 reg);

		// [DEBUG]
		void draw(const Point& pos);

	private:
		Memory* mem_;

		std::shared_ptr<APUStream> apuStream_;

		Audio audio_;

		Channel ch1_;
		Channel ch2_;
		Channel ch3_;
		Channel ch4_;

		// Frame Seq. Clock
		uint64 fsClock_ = 0;

		// Count T-cycles
		int cycles_ = 0;
		int cyclesMod_ = 95;

		uint8 prevDiv_ = 0;
	};
}
