#pragma once

#include "SquareChannel.h"
#include "WaveChannel.h"
#include "NoiseChannel.h"
#include "FrameSequencer.h"

namespace dmge
{
	struct APUStreamBufferState
	{
		int remain;
		int max;
	};


	class Memory;

	class Timer;

	class APUStream;

	class APU
	{
	public:
		APU(Timer& timer, int sampleRate = 44100);

		void setCGBMode(bool value);

		void setDoubleSpeed(bool value);

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

		// IOレジスタからの読み込み
		uint8 readRegister(uint16 addr) const;

		// NR52の下位4bit
		uint8 getChannelsEnabledState() const;

		// NR52
		void setMasterSwitch(uint8 NR52);

		// 現在のストリームバッファの状態を取得（デバッグ用）
		APUStreamBufferState getBufferState() const;

		// 各チャンネルの amplitude を取得
		std::array<int, 4> getAmplitude() const;

		// 各チャンネルのミュート状態を設定
		void setMute(int channel, bool mute);

		// 各チャンネルのミュート状態を設定
		bool getMute(int channel) const;

	private:
		Timer& timer_;

		int sampleRate_;

		std::shared_ptr<APUStream> apuStream_;

		Audio audio_;

		SquareChannel ch1_;
		SquareChannel ch2_;
		WaveChannel ch3_;
		NoiseChannel ch4_;

		FrameSequencer frameSeq_;

		// NR50, NR51

		uint8 nr50_ = 0;
		uint8 nr51_ = 0;

		// Master switch (NR52.7)
		bool masterSwitch_ = false;

		// Count T-cycles
		double cycles_ = 0;

		// CGB Mode
		bool cgbMode_ = false;

		// 各チャンネルのミュート状態
		std::array<int, 4> mute_;

		// 倍速モード時の DIV シフト量 (通常:0, 倍速:1)
		int divShiftBits_ = 0;

	};
}
