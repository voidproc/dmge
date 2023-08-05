#pragma once

#include "Test.h"

namespace dmge
{
	class Memory;
	class Interrupt;
	class CPU_detail;

	struct CPUState
	{
		uint16 af;
		uint16 bc;
		uint16 de;
		uint16 hl;
		uint16 sp;
		uint16 pc;
		bool halt;
		bool cgbMode;
	};

	class CPU
	{
	public:
		CPU(Memory* mem, Interrupt* interrupt);

		~CPU();

		void setCGBMode(bool enableCGBMode);

		void setSGBMode(bool enableSGBMode);

		// レジスタを初期状態にする
		void reset(bool enableBootROM);

		// CPU命令を１つフェッチ＆実行しPCを進める
		void run();

		// 割り込みが有効かつ割り込み要求があれば割り込みハンドラに制御を移す／条件によりHALTを解除する
		// 割り込みハンドラに制御を移した場合 true
		bool interrupt();

		// [DEBUG]現在の状態を出力
		void dump();

		// 現在のプログラムカウンタ(PC)
		uint16 currentPC() const;

		// run()により実行された命令が消費したクロック数
		int consumedCycles() const;

		// 現在のCPUの状態を取得（デバッグ用）
		CPUState getCurrentCPUState() const;

		MooneyeTestResult mooneyeTestResult() const;

	private:
		Memory* mem_;

		std::unique_ptr<CPU_detail> cpuDetail_;

	};
}
