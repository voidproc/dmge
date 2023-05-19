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
		void reset();
		void run();
		void interrupt();
		void applyScheduledIME();
		void dump();

		// Registers

		uint8 a;
		uint8 f;
		uint8 b;
		uint8 c;
		uint8 d;
		uint8 e;
		uint8 h;
		uint8 l;

		uint16 af() const
		{
			return (a << 8) | f;
		}

		void af(uint16 value)
		{
			a = value >> 8;
			f = value & 0xff;
		}

		uint16 bc() const
		{
			return (b << 8) | c;
		}

		void bc(uint16 value)
		{
			b = value >> 8;
			c = value & 0xff;
		}

		uint16 de() const
		{
			return (d << 8) | e;
		}

		void de(uint16 value)
		{
			d = value >> 8;
			e = value & 0xff;
		}

		uint16 hl() const
		{
			return (h << 8) | l;
		}

		void hl(uint16 value)
		{
			h = value >> 8;
			l = value & 0xff;
		}

		// Flags

		bool f_z() const
		{
			return f >> 7;
		}

		void f_z(bool value)
		{
			f = (f & (~(1 << 7))) | ((int)value << 7);
		}

		bool f_n() const
		{
			return (f >> 6) & 1;
		}

		void f_n(bool value)
		{
			f = (f & (~(1 << 6))) | ((int)value << 6);
		}

		bool f_h() const
		{
			return (f >> 5) & 1;
		}

		void f_h(bool value)
		{
			f = (f & (~(1 << 5))) | ((int)value << 5);
		}

		bool f_c() const
		{
			return (f >> 4) & 1;
		}

		void f_c(bool value)
		{
			f = (f & (~(1 << 4))) | ((int)value << 4);
		}

		// SP, PC
		uint16 sp;
		uint16 pc;

		// 次に実行する命令のあるアドレス
		uint16 addrNext;

		// 実際に消費したサイクル数
		int consumedCycles;

		// 指定したアドレスにある命令
		const Instruction& getInstruction(uint16 addr) const;

		// IME: Interrupt master enable flag
		bool ime = false;
		bool imeScheduled = false;

		// Interrupt
		bool prevStatInt_ = false;

		// HALT
		bool powerSavingMode_ = false;

	private:
		Memory* mem_;
		PPU* ppu_;
		LCD* lcd_;
		dmge::Timer* timer_;
	};
}
