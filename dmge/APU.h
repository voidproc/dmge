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

		int bufferRemain() const;

		int bufferMaxSize() const;

	private:
		virtual void getAudio(float* left, float* right, size_t samplesToWrite) override;

		virtual bool hasEnded() override;

		virtual void rewind() override;

	private:
		Wave wave_;

		int posHead_ = 0;
		int posTail_ = 0;

	};


	struct APUStreamBufferState
	{
		int remain;
		int max;
	};


	class APU
	{
	public:
		APU(Memory* mem, int sampleRate = 44100);

		// サウンド処理を1クロック分実行し、
		// サンプリングレートの周期にある場合はオーディオストリームにサンプルを書き込む
		// バッファに書き込んだサンプル数を返却する
		int run();

		// オーディオストリームのバッファリングがしきい値を超えている場合に再生を開始する
		void playIfBufferEnough(int thresholdSamples);

		// オーディオストリームのバッファリングがしきい値未満の場合に再生を一時停止する
		void pauseIfBufferNotEnough(int thresholdSamples);

		// オーディオストリームの再生を一時停止する
		void pause();

		// チャンネル操作

		void setEnable(Channels ch, bool enable);
		uint8 getEnableMask() const;
		void setFrequency(Channels ch, int freq);
		void setFrequencyLow(Channels ch, uint8 value);
		void setFrequencyHigh(Channels ch, uint8 value);
		void setEnvelopeAndDAC(Channels ch, uint8 reg);
		void setDAC(Channels ch, bool enable);
		void trigger(Channels ch);
		void setLengthTimer(Channels ch, uint8 reg);

		// 現在のストリームバッファの状態を取得（デバッグ用）
		APUStreamBufferState getBufferState() const;
		
	private:
		Memory* mem_;

		int sampleRate_;

		std::shared_ptr<APUStream> apuStream_;

		Audio audio_;

		Channel ch1_;
		Channel ch2_;
		Channel ch3_;
		Channel ch4_;

		// Frame Seq. Clock
		uint64 fsClock_ = 0;

		// Count T-cycles
		double cycles_ = 0;

		uint8 prevDiv_ = 0;
	};
}
