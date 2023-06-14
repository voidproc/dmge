#pragma once

#include "SquareChannel.h"
#include "WaveChannel.h"
#include "NoiseChannel.h"

namespace dmge
{
	struct APUStreamBufferState
	{
		int remain;
		int max;
	};


	class Memory;
	class APUStream;

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

		SquareChannel ch1_;
		SquareChannel ch2_;
		WaveChannel ch3_;
		NoiseChannel ch4_;

		// Frame Seq. Clock
		uint64 fsClock_ = 0;

		// Count T-cycles
		double cycles_ = 0;

		uint8 prevDiv_ = 0;
	};
}
