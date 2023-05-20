#pragma once

#include "Memory.h"

namespace dmge
{
	class CPU;

	struct Instruction
	{
		uint8 opcode;
		uint8 bytes;
		uint8 cycles;
		uint8 cyclesOnSkip;
		StringView mnemonic;
		StringView operands;
		void (*inst)(CPU*, Memory*, const Instruction*);
	};


	class PPU;
	class LCD;
	class Timer;

	class CPU
	{
	public:
		CPU(Memory* mem, PPU* ppu, LCD* lcd, dmge::Timer* timer);

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


		// レジスタ (A,F,B,C,D,E,H,L)

		uint8 a;  // レジスタA
		uint8 f;  // レジスタF
		uint8 b;  // レジスタB
		uint8 c;  // レジスタC
		uint8 d;  // レジスタD
		uint8 e;  // レジスタE
		uint8 h;  // レジスタH
		uint8 l;  // レジスタL

		// レジスタAF
		uint16 af() const
		{
			return (a << 8) | f;
		}

		// レジスタAFを設定
		void af(uint16 value)
		{
			a = value >> 8;
			f = value & 0xff;
		}

		// レジスタBC
		uint16 bc() const
		{
			return (b << 8) | c;
		}

		// レジスタBCを設定
		void bc(uint16 value)
		{
			b = value >> 8;
			c = value & 0xff;
		}

		// レジスタDE
		uint16 de() const
		{
			return (d << 8) | e;
		}

		// レジスタDEを設定
		void de(uint16 value)
		{
			d = value >> 8;
			e = value & 0xff;
		}

		// レジスタHL
		uint16 hl() const
		{
			return (h << 8) | l;
		}

		// レジスタHLを設定
		void hl(uint16 value)
		{
			h = value >> 8;
			l = value & 0xff;
		}


		// フラグ (Z,N,H,C)

		// ゼロフラグ
		bool f_z() const
		{
			return f >> 7;
		}

		// ゼロフラグを設定
		void f_z(bool value)
		{
			f = (f & (~(1 << 7))) | ((int)value << 7);
		}

		// Nフラグ
		bool f_n() const
		{
			return (f >> 6) & 1;
		}

		// Nフラグを設定
		void f_n(bool value)
		{
			f = (f & (~(1 << 6))) | ((int)value << 6);
		}

		// ハーフキャリーフラグ
		bool f_h() const
		{
			return (f >> 5) & 1;
		}

		// ハーフキャリーフラグを設定
		void f_h(bool value)
		{
			f = (f & (~(1 << 5))) | ((int)value << 5);
		}

		// キャリーフラグ
		bool f_c() const
		{
			return (f >> 4) & 1;
		}

		// キャリーフラグを設定
		void f_c(bool value)
		{
			f = (f & (~(1 << 4))) | ((int)value << 4);
		}


		// レジスタ (SP, PC)

		uint16 sp;  // レジスタSP
		uint16 pc;  // レジスタPC


		// 次に実行する命令のあるアドレス
		uint16 addrNext;

		// 実際に消費したサイクル数
		int consumedCycles;

		// 指定したアドレスにある命令
		const Instruction& getInstruction(uint16 addr) const;

		// IME: Interrupt master enable flag
		bool ime = false;
		bool imeScheduled = false;

		// HALT
		bool powerSavingMode_ = false;

	private:
		Memory* mem_;
		PPU* ppu_;
		LCD* lcd_;
		dmge::Timer* timer_;
	};
}
