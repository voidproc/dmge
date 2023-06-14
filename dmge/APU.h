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

		// IOレジスタへの書き込み
		void writeRegister(uint16 addr, uint8 value);

		// NR52の下位4bit
		uint8 getChannelsEnabledState() const;

		// 現在のストリームバッファの状態を取得（デバッグ用）
		APUStreamBufferState getBufferState() const;
		
	private:
		Memory* mem_;

		int sampleRate_;

		std::shared_ptr<APUStream> apuStream_;

		Audio audio_;

		SquareChannel ch1__;
		SquareChannel ch2__;
		WaveChannel ch3__;
		NoiseChannel ch4__;

		// Frame Seq. Clock
		uint64 fsClock_ = 0;

		// Count T-cycles
		double cycles_ = 0;

		uint8 prevDiv_ = 0;
	};
}
