#pragma once

namespace dmge
{
	class Memory;
	class CPU_detail;

	struct RegisterValues
	{
		uint16 af;
		uint16 bc;
		uint16 de;
		uint16 hl;
		uint16 sp;
		uint16 pc;
	};

	class CPU
	{
	public:
		CPU(Memory* mem);

		~CPU();

		// レジスタを初期状態にする
		void reset();

		// CPU命令を１つフェッチ＆実行しPCを進める
		void run();

		// 割り込みが有効かつ割り込み要求があれば実行する
		void interrupt();

		// IE命令の遅延のため(?)
		void applyScheduledIME();

		// [DEBUG]現在の状態を出力
		void dump();

		// 現在のプログラムカウンタ(PC)
		uint16 currentPC() const;

		// run()により実行された命令が消費したクロック数
		int consumedCycles() const;

		// 現在のレジスタの値を取得（デバッグ用）
		RegisterValues getRegisterValues() const;

	private:
		Memory* mem_;

		std::unique_ptr<CPU_detail> cpuDetail_;

	};
}
