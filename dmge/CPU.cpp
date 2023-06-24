#include "CPU.h"
#include "Memory.h"
#include "Interrupt.h"
#include "DebugPrint.h"

namespace dmge
{
	struct Instruction
	{
		uint8 opcode;
		uint8 bytes;
		uint8 cycles;
		uint8 cyclesOnSkip;
		StringView mnemonic;
		StringView operands;
		void (CPU_detail::* inst)(Memory*, const Instruction*);
	};


	class CPU_detail
	{
	public:
		friend CPU;

		CPU_detail(Memory* mem, Interrupt* interrupt)
			: mem_{ mem }, interrupt_{ interrupt }
		{
		}

		void setCGBMode(bool value)
		{
			cgbMode_ = value;
		}

		void reset()
		{
			af(cgbMode_ ? 0x11b0 : 0x01b0); // GB/SGB:0x01b0, GBP:0xffb0, GBC:0x11b0
			bc(cgbMode_ ? 0x0000 : 0x0013);
			de(0x00d8);
			hl(0x014d);
			sp = 0xfffe;
			pc = 0x0100;
		}

		void run()
		{
			// HALTによって低電力モードになっている場合はPCからのフェッチ＆実行をしない
			if (powerSavingMode_)
			{
				return;
			}

			// IMEがスケジュールされていたら有効化
			interrupt_->updateIME();

			// 現在のPCの命令をフェッチし、
			// 次のPCと消費サイクルを計算、
			// フェッチした命令を実行
			// ※実行した結果、消費サイクルが書き変わる場合は consumedCycles_ が変更されている（ジャンプ命令でジャンプしなかった場合など）
			// ※実行した結果、次のPCが書き変わる場合は pcNext_ が変更されているので PC に反映する（JPやCALLなど）

			const auto& instruction = getInstruction_(pc);

			pcNext_ = pc + instruction.bytes;
			consumedCycles_ = instruction.cycles;

			if (instruction.inst != nullptr)
			{
				(this->*(instruction.inst))(mem_, &instruction);
			}

			pc = pcNext_;
		}

		void interrupt()
		{
			// 低電力モードから抜ける？

			if (not interrupt_->ime())
			{
				if (interrupt_->requested())
				{
					powerSavingMode_ = false;
				}

				return;
			}

			// 割り込みを実行

			const std::array<uint16, 5> intAddr = {
				Address::INT40_VBlank,
				Address::INT48_STAT,
				Address::INT50_Timer,
				Address::INT58_Serial,
				Address::INT60_Joypad,
			};

			for (int i : step(5))
			{
				if (interrupt_->requested(i))
				{
					// 割り込みを無効に
					interrupt_->disableIME();
					interrupt_->disableInterruptFlag(i);

					// 現在のPCをスタックにプッシュし、割り込みベクタにジャンプ
					mem_->write(--sp, pc >> 8);
					mem_->write(--sp, pc & 0xff);
					pc = intAddr[i];

					// 低電力モードから抜ける
					powerSavingMode_ = false;

					// 割り込みを１つ実行して、この回は終了
					break;
				}
			}
		}

		void dump()
		{
			const auto& inst = getInstruction_(pc);

			const uint8 ly = mem_->read(Address::LY);
			const uint8 stat = mem_->read(Address::STAT);
			const uint8 tima = mem_->read(Address::TIMA);
			const uint8 tma = mem_->read(Address::TMA);
			const uint8 tac = mem_->read(Address::TAC);
			const uint8 intEnable = mem_->read(Address::IE);
			const uint8 intFlag = mem_->read(Address::IF);

			DebugPrint::Writeln(U"pc:{:04X} {:5} {:10} af:{:04X} bc:{:04X} de:{:04X} hl:{:04X} sp:{:04X} ly:{:02X} stat:{:02X} tima:{:02X} tma:{:02X} tac:{:02X} ie:{:02X} if:{:02X} rom:{:02X}"_fmt(
				pc,
				inst.mnemonic, inst.operands,
				af(), bc(), de(), hl(), sp,
				ly, stat, tima, tma, tac, intEnable, intFlag, mem_->romBank()
			));
		}

	private:
		Memory* mem_;

		Interrupt* interrupt_;

		// レジスタ (A,F,B,C,D,E,H,L)

		uint8 a = 0;  // レジスタA
		uint8 f = 0;  // レジスタF
		uint8 b = 0;  // レジスタB
		uint8 c = 0;  // レジスタC
		uint8 d = 0;  // レジスタD
		uint8 e = 0;  // レジスタE
		uint8 h = 0;  // レジスタH
		uint8 l = 0;  // レジスタL

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

		uint16 sp = 0;  // レジスタSP
		uint16 pc = 0;  // レジスタPC

		// 次に実行する命令のあるアドレス
		uint16 pcNext_ = 0;

		// 実際に消費したサイクル数
		int consumedCycles_ = 0;

		// IME: Interrupt master enable flag
		//bool ime_ = false;
		//bool imeScheduled_ = false;

		// HALT
		bool powerSavingMode_ = false;

		// CGB Mode
		bool cgbMode_ = false;


		// 命令セット
		
		// Loads
		// (LD, LDI, LDD, LDH, PUSH, POP)

		void ld_r_n_(Memory* mem, const Instruction* inst)
		{
			const uint8 n = mem->read(pc + 1);

			switch (inst->opcode)
			{
			case 0x06: b = n; return;
			case 0x0e: c = n; return;
			case 0x16: d = n; return;
			case 0x1e: e = n; return;
			case 0x26: h = n; return;
			case 0x2e: l = n; return;
			default: return;
			}
		}

		void ld_r_r_(Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
				// A
				//case 0x7f: return;
				//case 0x78: return;
				//case 0x79: return;
				//case 0x7a: return;
				//case 0x7b: return;
				//case 0x7c: return;
				//case 0x7d: return;
				//case 0x7e: return;

				// B
			case 0x40: b = b; return;
			case 0x41: b = c; return;
			case 0x42: b = d; return;
			case 0x43: b = e; return;
			case 0x44: b = h; return;
			case 0x45: b = l; return;
			case 0x46: b = mem->read(hl()); return;

				// C
			case 0x48: c = b; return;
			case 0x49: c = c; return;
			case 0x4a: c = d; return;
			case 0x4b: c = e; return;
			case 0x4c: c = h; return;
			case 0x4d: c = l; return;
			case 0x4e: c = mem->read(hl()); return;

				// D
			case 0x50: d = b; return;
			case 0x51: d = c; return;
			case 0x52: d = d; return;
			case 0x53: d = e; return;
			case 0x54: d = h; return;
			case 0x55: d = l; return;
			case 0x56: d = mem->read(hl()); return;

				// E
			case 0x58: e = b; return;
			case 0x59: e = c; return;
			case 0x5a: e = d; return;
			case 0x5b: e = e; return;
			case 0x5c: e = h; return;
			case 0x5d: e = l; return;
			case 0x5e: e = mem->read(hl()); return;

				//H
			case 0x60: h = b; return;
			case 0x61: h = c; return;
			case 0x62: h = d; return;
			case 0x63: h = e; return;
			case 0x64: h = h; return;
			case 0x65: h = l; return;
			case 0x66: h = mem->read(hl()); return;

				// L
			case 0x68: l = b; return;
			case 0x69: l = c; return;
			case 0x6a: l = d; return;
			case 0x6b: l = e; return;
			case 0x6c: l = h; return;
			case 0x6d: l = l; return;
			case 0x6e: l = mem->read(hl()); return;

				// (HL)
			case 0x70: mem->write(hl(), b); return;
			case 0x71: mem->write(hl(), c); return;
			case 0x72: mem->write(hl(), d); return;
			case 0x73: mem->write(hl(), e); return;
			case 0x74: mem->write(hl(), h); return;
			case 0x75: mem->write(hl(), l); return;
			case 0x36: mem->write(hl(), mem->read(pc + 1)); return;

			default: return;
			}
		}

		void ld_a_n_(Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0x7f: return;
			case 0x78: n = b; break;
			case 0x79: n = c; break;
			case 0x7a: n = d; break;
			case 0x7b: n = e; break;
			case 0x7c: n = h; break;
			case 0x7d: n = l; break;
			case 0x0a: n = mem->read(bc()); break;
			case 0x1a: n = mem->read(de()); break;
			case 0x7e: n = mem->read(hl()); break;
			case 0xfa: n = mem->read(mem->read16(pc + 1)); break;
			case 0x3e: n = mem->read(pc + 1); break;
			default: return;
			}

			a = n;
		}

		void ld_n_a_(Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x7f: a = a; return;
			case 0x47: b = a; return;
			case 0x4f: c = a; return;
			case 0x57: d = a; return;
			case 0x5f: e = a; return;
			case 0x67: h = a; return;
			case 0x6f: l = a; return;
			case 0x02: mem->write(bc(), a); return;
			case 0x12: mem->write(de(), a); return;
			case 0x77: mem->write(hl(), a); return;
			case 0xea: mem->write(mem->read16(pc + 1), a); return;
			default: return;
			}
		}

		void ld_a_c_(Memory* mem, const Instruction*)
		{
			// opcode: 0xf2
			a = mem->read(0xff00 + c);
		}

		void ld_c_a_(Memory* mem, const Instruction*)
		{
			// opcode: 0xe2
			mem->write(0xff00 + c, a);
		}

		void ldd_a_hl_(Memory* mem, const Instruction* inst)
		{
			// opcode: 0x3a
			a = mem->read(hl());
			dec16_(mem, inst);
		}

		void ldd_hl_a_(Memory* mem, const Instruction* inst)
		{
			// opcode: 0x32
			mem->write(hl(), a);
			dec16_(mem, inst);
		}

		void ldi_a_hl_(Memory* mem, const Instruction* inst)
		{
			// opcode: 0x2a
			a = mem->read(hl());
			inc16_(mem, inst);
		}

		void ldi_hl_a_(Memory* mem, const Instruction* inst)
		{
			// opcode: 0x22
			mem->write(hl(), a);
			inc16_(mem, inst);
		}

		void ldh_(Memory* mem, const Instruction* inst)
		{
			const uint8 n = mem->read(pc + 1);

			switch (inst->opcode)
			{
			case 0xe0:
				mem->write(0xff00 + n, a);
				return;
			case 0xf0:
				a = mem->read(0xff00 + n);
				return;
			default:
				return;
			}
		}

		void ld16_(Memory* mem, const Instruction* inst)
		{
			const uint16 n = mem->read16(pc + 1);

			switch (inst->opcode)
			{
			case 0x01: bc(n); return;
			case 0x11: de(n); return;
			case 0x21: hl(n); return;
			case 0x31: sp = n; return;
			default: return;
			}
		}

		void ld16_sp_hl_(Memory*, const Instruction*)
		{
			// opcode: 0xf9
			sp = hl();
		}

		void ldhl_sp_n_(Memory* mem, const Instruction*)
		{
			// opcode: 0xf8

			const int8 n = mem->read(pc + 1);

			hl(sp + n);  //※符号付演算

			f_z(false);
			f_n(false);
			f_h((sp & 0xf) + (n & 0xf) > 0xf);
			f_c((sp & 0xff) + (n & 0xff) > 0xff);
		}

		void ld16_n_sp_(Memory* mem, const Instruction*)
		{
			// opcode: 0x08
			const uint16 addr = mem->read16(pc + 1);
			mem->write(addr, sp & 0xff);
			mem->write(addr + 1, (sp >> 8) & 0xff);
		}

		void push_(Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0xf5:
				mem->write(--sp, a);
				mem->write(--sp, f);
				return;

			case 0xc5:
				mem->write(--sp, b);
				mem->write(--sp, c);
				return;

			case 0xd5:
				mem->write(--sp, d);
				mem->write(--sp, e);
				return;

			case 0xe5:
				mem->write(--sp, h);
				mem->write(--sp, l);
				return;

			default: return;
			}
		}

		void pop_(Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0xf1:
				af(mem->read16(sp++) & 0xfff0);
				break;

			case 0xc1:
				bc(mem->read16(sp++));
				break;

			case 0xd1:
				de(mem->read16(sp++));
				break;

			case 0xe1:
				hl(mem->read16(sp++));
				break;
			}

			++sp;
		}

		// Arithmetic
		// (ADD, ADC, SUB, SBC, AND, OR, XOR, CP, INC, DEC)

		void add_a_(Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x87: n = a; break;
			case 0x80: n = b; break;
			case 0x81: n = c; break;
			case 0x82: n = d; break;
			case 0x83: n = e; break;
			case 0x84: n = h; break;
			case 0x85: n = l; break;
			case 0x86: n = mem->read(hl()); break;
			case 0xc6: n = mem->read(pc + 1); break;
			default: return;
			}

			f_n(false);
			f_h((a & 0xf) + (n & 0xf) > 0xf);
			f_c((a & 0xff) + (n & 0xff) > 0xff);
			a += n;
			f_z(a == 0);
		}

		void adc_a_(Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x8f: n = a; break;
			case 0x88: n = b; break;
			case 0x89: n = c; break;
			case 0x8a: n = d; break;
			case 0x8b: n = e; break;
			case 0x8c: n = h; break;
			case 0x8d: n = l; break;
			case 0x8e: n = mem->read(hl()); break;
			case 0xce: n = mem->read(pc + 1); break;
			default: return;
			}

			const uint8 carry = (uint8)f_c();

			f_n(false);
			f_h((a & 0xf) + (n & 0xf) + carry > 0xf);
			f_c((a & 0xff) + (n & 0xff) + carry > 0xff);
			a += n + carry;
			f_z(a == 0);
		}

		void sub_a_(Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x97: n = a; break;
			case 0x90: n = b; break;
			case 0x91: n = c; break;
			case 0x92: n = d; break;
			case 0x93: n = e; break;
			case 0x94: n = h; break;
			case 0x95: n = l; break;
			case 0x96: n = mem->read(hl()); break;
			case 0xd6: n = mem->read(pc + 1); break;
			default: return;
			}

			f_n(true);
			f_h((a & 0xf) - (n & 0xf) < 0);
			f_c(a < n);
			a -= n;
			f_z(a == 0);
		}

		void sbc_a_(Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x9f: n = a; break;
			case 0x98: n = b; break;
			case 0x99: n = c; break;
			case 0x9a: n = d; break;
			case 0x9b: n = e; break;
			case 0x9c: n = h; break;
			case 0x9d: n = l; break;
			case 0x9e: n = mem->read(hl()); break;
			case 0xde: n = mem->read(pc + 1); break;
			default: return;
			}

			const uint8 carry = (uint8)f_c();

			f_n(true);
			f_h((a & 0xf) - ((n & 0xf) + carry) < 0);
			f_c(a < (n + carry));
			a -= n + carry;
			f_z(a == 0);
		}

		void and_(Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xa7: n = a; break;
			case 0xa0: n = b; break;
			case 0xa1: n = c; break;
			case 0xa2: n = d; break;
			case 0xa3: n = e; break;
			case 0xa4: n = h; break;
			case 0xa5: n = l; break;
			case 0xa6: n = mem->read(hl()); break;
			case 0xe6: n = mem->read(pc + 1); break;
			default: return;
			}

			a &= n;

			f_z(a == 0);
			f_n(false);
			f_h(true);
			f_c(false);
		}

		void or_(Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xb7: n = a; break;
			case 0xb0: n = b; break;
			case 0xb1: n = c; break;
			case 0xb2: n = d; break;
			case 0xb3: n = e; break;
			case 0xb4: n = h; break;
			case 0xb5: n = l; break;
			case 0xb6: n = mem->read(hl()); break;
			case 0xf6: n = mem->read(pc + 1); break;
			default: return;
			}

			a |= n;

			f_z(a == 0);
			f_n(false);
			f_h(false);
			f_c(false);
		}

		void xor_(Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xaf: n = a; break;
			case 0xa8: n = b; break;
			case 0xa9: n = c; break;
			case 0xaa: n = d; break;
			case 0xab: n = e; break;
			case 0xac: n = h; break;
			case 0xad: n = l; break;
			case 0xae: n = mem->read(hl()); break;
			case 0xee: n = mem->read(pc + 1); break;
			default: return;
			}

			a ^= n;

			f_z(a == 0);
			f_n(false);
			f_h(false);
			f_c(false);
		}

		void cp_(Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xbf: n = a; break;
			case 0xb8: n = b; break;
			case 0xb9: n = c; break;
			case 0xba: n = d; break;
			case 0xbb: n = e; break;
			case 0xbc: n = h; break;
			case 0xbd: n = l; break;
			case 0xbe: n = mem->read(hl()); break;
			case 0xfe: n = mem->read(pc + 1); break;
			default: return;
			}

			f_z(a == n);
			f_n(true);
			f_h((a & 0xf) - (n & 0xf) < 0);
			f_c(a < n);
		}

		void inc_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3c: p = &a; break;
			case 0x04: p = &b; break;
			case 0x0c: p = &c; break;
			case 0x14: p = &d; break;
			case 0x1c: p = &e; break;
			case 0x24: p = &h; break;
			case 0x2c: p = &l; break;
			default: return;
			}

			f_n(false);
			f_h((*p & 0xf) == 0xf);

			const uint8 result = *p + 1;

			*p = result;
			f_z(result == 0);
		}

		void inc_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x34
			uint8 value = mem->read(hl());

			f_n(false);
			f_h((value & 0xf) == 0xf);

			const uint8 result = value + 1;

			mem->write(hl(), result);
			f_z(result == 0);
		}

		void dec_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3d: p = &a; break;
			case 0x05: p = &b; break;
			case 0x0d: p = &c; break;
			case 0x15: p = &d; break;
			case 0x1d: p = &e; break;
			case 0x25: p = &h; break;
			case 0x2d: p = &l; break;
			default: return;
			}

			f_n(true);
			f_h((*p & 0xf) == 0);

			const uint8 result = *p - 1;

			*p = result;

			f_z(result == 0);
		}

		void dec_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x35
			uint8 value = mem->read(hl());

			f_n(true);
			f_h((value & 0xf) == 0);

			const uint8 result = value - 1;

			mem->write(hl(), result);
			f_z(result == 0);
		}

		void add_hl_(Memory*, const Instruction* inst)
		{
			uint16 n;

			switch (inst->opcode)
			{
			case 0x09: n = bc(); break;
			case 0x19: n = de(); break;
			case 0x29: n = hl(); break;
			case 0x39: n = sp; break;
			default: return;
			}

			f_n(false);
			f_h((hl() & 0x7ff) + (n & 0x7ff) > 0x7ff);
			f_c((hl() & 0xffff) + (n & 0xffff) > 0xffff);
			hl(hl() + n);
		}

		void add_sp_(Memory* mem, const Instruction*)
		{
			// opcode: 0xe8
			const int8 n = mem->read(pc + 1);
			f = 0;
			f_h((sp & 0xf) + (n & 0xf) > 0xf);
			f_c((sp & 0xff) + (n & 0xff) > 0xff);
			sp += n;
		}

		void inc16_(Memory*, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x03: bc(bc() + 1); return;
			case 0x13: de(de() + 1); return;

			case 0x22: //LDI (HL),A
			case 0x2a: //LDI A,(HL)
			case 0x23: hl(hl() + 1); return;

			case 0x33: ++sp; return;
			}
		}

		void dec16_(Memory*, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x0b: bc(bc() - 1); return;
			case 0x1b: de(de() - 1); return;

			case 0x32: // LDD (HL),A
			case 0x3a: // LDD A,(HL)
			case 0x2b: hl(hl() - 1); return;

			case 0x3b: --sp; return;
			default: return;
			}
		}

		// Misc
		// (SWAP, DAA, CPL, CCF, SCF, NOP, HALT, STOP, DI, EI)

		void swap_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x37: p = &a; break;
			case 0x30: p = &b; break;
			case 0x31: p = &c; break;
			case 0x32: p = &d; break;
			case 0x33: p = &e; break;
			case 0x34: p = &h; break;
			case 0x35: p = &l; break;
			default: return;
			}

			const uint8 result = (*p >> 4) | (*p << 4);

			*p = result;

			f = 0;
			f_z(result == 0);
		}

		void swap_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x36
			uint8 value = mem->read(hl());

			const uint8 result = (value >> 4) | (value << 4);

			mem->write(hl(), result);

			f = 0;
			f_z(result == 0);
		}

		void daa_(Memory*, const Instruction*)
		{
			// opcode: 0x27
			int16 result = a;

			if (f_n())
			{
				if (f_h())
				{
					result = (result - 0x6) & 0xff;
				}

				if (f_c())
				{
					result -= 0x60;
				}
			}
			else
			{
				if (f_h() || (result & 0xf) > 0x9)
				{
					result += 0x6;
				}

				if (f_c() || result > 0x9f)
				{
					result += 0x60;
				}
			}

			f_c(f_c() || ((result & 0x100) == 0x100));

			a = result & 0xff;

			f_z(a == 0);
			f_h(false);
		}

		void cpl_(Memory*, const Instruction*)
		{
			// opcode: 0x2f
			a ^= 0xff;
			f_n(true);
			f_h(true);
		}

		void ccf_(Memory*, const Instruction*)
		{
			// opcode: 0x3f
			f_c(!f_c());
			f_n(false);
			f_h(false);
		}

		void scf_(Memory*, const Instruction*)
		{
			// opcode: 0x37
			f_c(true);
			f_n(false);
			f_h(false);
		}

		void nop_(Memory*, const Instruction*)
		{
			// opcode: 0x00
		}

		void halt_(Memory* mem, const Instruction*)
		{
			// opcode: 0x76

			// HALT
			// 割り込みの状況によって低電力モードになったりならなかったりする

			if (interrupt_->ime())
			{
				powerSavingMode_ = true;
			}
			else
			{
				const uint8 intEnable = mem->read(Address::IE);
				const uint8 intFlag = mem->read(Address::IF);
				if ((intEnable & intFlag) != 0)
				{
					;
				}
				else
				{
					powerSavingMode_ = true;
				}
			}
		}

		void stop_(Memory*, const Instruction*)
		{
			// opcode: 0x10
			//...
		}

		void di_(Memory*, const Instruction*)
		{
			// opcode: 0xf3
			interrupt_->disableIME();
		}

		void ei_(Memory*, const Instruction*)
		{
			// opcode: 0xfb
			interrupt_->reserveEnablingIME();
		}

		// Rotates & Shifts
		// (RLCA, RLA, RRCA, RRA, RLC, RL, RRA, RR)

		void rlca_(Memory*, const Instruction*)
		{
			// opcode: 0x07
			const uint8 bit7 = a >> 7;
			f = 0;
			f_c(bit7 == 1);
			a = (a << 1) | bit7;
		}

		void rla_(Memory*, const Instruction*)
		{
			// opcode: 0x17
			const uint8 bit7 = a >> 7;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit7 == 1);
			a = (a << 1) | carry;
		}

		void rrca_(Memory*, const Instruction*)
		{
			// opcode: 0x0f
			const uint8 bit0 = a & 1;
			f = 0;
			f_c(bit0 == 1);
			a = (a >> 1) | (bit0 << 7);
		}

		void rra_(Memory*, const Instruction*)
		{
			// opcode: 0x1f
			const uint8 bit0 = a & 1;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit0 == 1);
			a = (a >> 1) | (carry << 7);
		}

		void rlc_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x07: p = &a; break;
			case 0x00: p = &b; break;
			case 0x01: p = &c; break;
			case 0x02: p = &d; break;
			case 0x03: p = &e; break;
			case 0x04: p = &h; break;
			case 0x05: p = &l; break;
			default: return;
			}

			const uint8 bit7 = *p >> 7;
			f = 0;
			f_z(*p == 0);
			f_c(bit7 == 1);

			const uint8 result = (*p << 1) | bit7;

			*p = result;
		}

		void rlc_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x06
			uint8 value = mem->read(hl());

			const uint8 bit7 = value >> 7;
			f = 0;
			f_z(value == 0);
			f_c(bit7 == 1);

			const uint8 result = (value << 1) | bit7;

			mem->write(hl(), result);
		}

		void rl_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x17: p = &a; break;
			case 0x10: p = &b; break;
			case 0x11: p = &c; break;
			case 0x12: p = &d; break;
			case 0x13: p = &e; break;
			case 0x14: p = &h; break;
			case 0x15: p = &l; break;
			default: return;
			}

			const uint8 bit7 = *p >> 7;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit7 == 1);

			const uint8 result = (*p << 1) | carry;

			*p = result;

			f_z(result == 0);
		}

		void rl_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x16
			uint8 value = mem->read(hl());

			const uint8 bit7 = value >> 7;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit7 == 1);

			const uint8 result = (value << 1) | carry;

			mem->write(hl(), result);

			f_z(result == 0);
		}

		void rrc_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x0f: p = &a; break;
			case 0x08: p = &b; break;
			case 0x09: p = &c; break;
			case 0x0a: p = &d; break;
			case 0x0b: p = &e; break;
			case 0x0c: p = &h; break;
			case 0x0d: p = &l; break;
			default: return;
			}

			const uint8 bit0 = *p & 1;
			f = 0;
			f_c(bit0 == 1);

			const uint8 result = (*p >> 1) | (bit0 << 7);

			*p = result;

			f_z(*p == 0);
		}

		void rrc_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x0e
			uint8 value = mem->read(hl());

			const uint8 bit0 = value & 1;
			f = 0;
			f_c(bit0 == 1);

			const uint8 result = (value >> 1) | (bit0 << 7);

			mem->write(hl(), result);

			f_z(result == 0);
		}

		void rr_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x1f: p = &a; break;
			case 0x18: p = &b; break;
			case 0x19: p = &c; break;
			case 0x1a: p = &d; break;
			case 0x1b: p = &e; break;
			case 0x1c: p = &h; break;
			case 0x1d: p = &l; break;
			default: return;
			}

			const uint8 bit0 = *p & 1;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit0 == 1);

			const uint8 result = (*p >> 1) | (carry << 7);

			*p = result;

			f_z(result == 0);
		}

		void rr_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x1e
			uint8 value = mem->read(hl());

			const uint8 bit0 = value & 1;
			const uint8 carry = (uint8)f_c();
			f = 0;
			f_c(bit0 == 1);

			const uint8 result = (value >> 1) | (carry << 7);

			mem->write(hl(), result);

			f_z(result == 0);
		}

		void sla_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x27: p = &a; break;
			case 0x20: p = &b; break;
			case 0x21: p = &c; break;
			case 0x22: p = &d; break;
			case 0x23: p = &e; break;
			case 0x24: p = &h; break;
			case 0x25: p = &l; break;
			default: return;
			}

			f = 0;
			f_c(*p & (1 << 7));

			const uint8 result = *p << 1;

			*p = result;

			f_z(result == 0);
		}

		void sla_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x26
			uint8 value = mem->read(hl());

			f = 0;
			f_c(value & (1 << 7));

			const uint8 result = value << 1;

			mem->write(hl(), result);

			f_z(result == 0);
		}

		void sra_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x2f: p = &a; break;
			case 0x28: p = &b; break;
			case 0x29: p = &c; break;
			case 0x2a: p = &d; break;
			case 0x2b: p = &e; break;
			case 0x2c: p = &h; break;
			case 0x2d: p = &l; break;
			default: return;
			}

			f = 0;
			f_c(*p & 1);

			const uint8 result = (*p >> 1) | (*p & (1 << 7));

			*p = result;

			f_z(result == 0);
		}

		void sra_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x2e
			uint8 value = mem->read(hl());

			f = 0;
			f_c(value & 1);

			const uint8 result = (value >> 1) | (value & (1 << 7));

			mem->write(hl(), result);

			f_z(result == 0);
		}

		void srl_(Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3f: p = &a; break;
			case 0x38: p = &b; break;
			case 0x39: p = &c; break;
			case 0x3a: p = &d; break;
			case 0x3b: p = &e; break;
			case 0x3c: p = &h; break;
			case 0x3d: p = &l; break;
			default: return;
			}

			f = 0;
			f_c(*p & 1);

			const uint8 result = *p >> 1;

			*p = result;

			f_z(result == 0);
		}

		void srl_hl_(Memory* mem, const Instruction*)
		{
			// opcode: 0x3e
			uint8 value = mem->read(hl());

			f = 0;
			f_c(value & 1);

			const uint8 result = value >> 1;

			mem->write(hl(), result);

			f_z(result == 0);
		}

		// Bit
		// (BIT, SET)

		void bit_(Memory*, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0x40) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &a; break;
			case 0x0: p = &b; break;
			case 0x1: p = &c; break;
			case 0x2: p = &d; break;
			case 0x3: p = &e; break;
			case 0x4: p = &h; break;
			case 0x5: p = &l; break;
			default: return;
			}

			f_z((*p & (1 << bit)) == 0);
			f_n(false);
			f_h(true);
		}

		void bit_hl_(Memory* mem, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0x40) / 8;

			uint8 value = mem->read(hl());

			f_z((value & (1 << bit)) == 0);
			f_n(false);
			f_h(true);
		}

		void set_(Memory*, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0xc0) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &a; break;
			case 0x0: p = &b; break;
			case 0x1: p = &c; break;
			case 0x2: p = &d; break;
			case 0x3: p = &e; break;
			case 0x4: p = &h; break;
			case 0x5: p = &l; break;
			default: return;
			}

			const uint8 result = *p | (1 << bit);

			*p = result;
		}

		void set_hl_(Memory* mem, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0xc0) / 8;

			uint8 value = mem->read(hl());

			const uint8 result = value | (1 << bit);

			mem->write(hl(), result);
		}

		void res_(Memory*, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0x80) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &a; break;
			case 0x0: p = &b; break;
			case 0x1: p = &c; break;
			case 0x2: p = &d; break;
			case 0x3: p = &e; break;
			case 0x4: p = &h; break;
			case 0x5: p = &l; break;
			default: return;
			}

			const uint8 result = *p & ~(1 << bit);

			*p = result;
		}

		void res_hl_(Memory* mem, const Instruction* inst)
		{
			const uint8 bit = (inst->opcode - 0x80) / 8;

			uint8 value = mem->read(hl());

			const uint8 result = value & ~(1 << bit);

			mem->write(hl(), result);
		}

		// Jumps
		// (JP, JR)

		void jp_(Memory* mem, const Instruction* inst)
		{
			const uint16 dstAddr = mem->read16(pc + 1);
			bool toJump = false;

			switch (inst->opcode)
			{
			case 0xc3:
				toJump = true;
				break;
			case 0xc2:
				if (not f_z()) toJump = true;
				break;
			case 0xca:
				if (f_z()) toJump = true;
				break;
			case 0xd2:
				if (not f_c()) toJump = true;
				break;
			case 0xda:
				if (f_c()) toJump = true;
				break;
			}

			if (toJump)
			{
				pcNext_ = dstAddr;
				return;
			}

			consumedCycles_ = inst->cyclesOnSkip;
		}

		void jp_hl_(Memory*, const Instruction*)
		{
			// opcode: 0xe9
			pcNext_ = hl();
		}

		void jr_(Memory* mem, const Instruction* inst)
		{
			const int8 n = mem->read(pc + 1);
			bool toJump = false;

			switch (inst->opcode)
			{
			case 0x18:
				toJump = true;
				break;
			case 0x20:
				if (not f_z()) toJump = true;
				break;
			case 0x28:
				if (f_z()) toJump = true;
				break;
			case 0x30:
				if (not f_c()) toJump = true;
				break;
			case 0x38:
				if (f_c()) toJump = true;
				break;
			}

			if (toJump)
			{
				pcNext_ = pcNext_ + n;
				return;
			}

			consumedCycles_ = inst->cyclesOnSkip;
		}

		// Calls
		// (CALL)

		void call_(Memory* mem, const Instruction* inst)
		{
			bool toCall = false;

			switch (inst->opcode)
			{
			case 0xcd:
				toCall = true;
				break;
			case 0xc4:
				if (not f_z()) toCall = true;
				break;
			case 0xcc:
				if (f_z()) toCall = true;
				break;
			case 0xd4:
				if (not f_c()) toCall = true;
				break;
			case 0xdc:
				if (f_c()) toCall = true;
				break;
			}

			if (toCall)
			{
				const uint16 addr = mem->read16(pc + 1);
				mem->write(--sp, (pc + 3) >> 8);
				mem->write(--sp, (pc + 3) & 0xff);
				pcNext_ = addr;
				return;
			}

			consumedCycles_ = inst->cyclesOnSkip;
		}

		// Restarts
		// (RST)

		void rst_(Memory* mem, const Instruction* inst)
		{
			uint16 addr;

			switch (inst->opcode)
			{
			case 0xc7: addr = 0x00; break;
			case 0xcf: addr = 0x08; break;
			case 0xd7: addr = 0x10; break;
			case 0xdf: addr = 0x18; break;
			case 0xe7: addr = 0x20; break;
			case 0xef: addr = 0x28; break;
			case 0xf7: addr = 0x30; break;
			case 0xff: addr = 0x38; break;
			default: return;
			}

			mem->write(--sp, (pc + 1) >> 8);
			mem->write(--sp, (pc + 1) & 0xff);
			pcNext_ = addr;
		}


		// Returns
		// (RET, RETI)

		void ret_(Memory* mem, const Instruction* inst)
		{
			bool toRet = false;

			switch (inst->opcode)
			{
			case 0xc9:
				toRet = true;
				break;
			case 0xc0:
				if (not f_z()) toRet = true;
				break;
			case 0xc8:
				if (f_z()) toRet = true;
				break;
			case 0xd0:
				if (not f_c()) toRet = true;
				break;
			case 0xd8:
				if (f_c()) toRet = true;
				break;
			}

			if (toRet)
			{
				pcNext_ = mem->read16(sp);
				sp += 2;
				return;
			}

			consumedCycles_ = inst->cyclesOnSkip;
		}

		void reti_(Memory* mem, const Instruction*)
		{
			// opcode: 0xd9
			pcNext_ = mem->read16(sp);
			sp += 2;
			interrupt_->reserveEnablingIME(); //?
		}

		// Other

		void cbprefix_(Memory*, const Instruction*)
		{
		}

		void illegal_(Memory*, const Instruction*)
		{
		}

		// 指定したアドレスにある命令を得る
		const Instruction& getInstruction_(uint16 addr)
		{
			const uint8 code = mem_->read(addr);
			if (code != 0xcb) return unprefixedInstructions[code];

			return cbprefixedInstructions[mem_->read(addr + 1)];
		}

		// Instructions List 0x00-0xff

		std::array<Instruction, 256> unprefixedInstructions = {
			{
				{ 0x00, 1, 4, 0, U"NOP"_sv, U""_sv, &CPU_detail::nop_ },
				{ 0x01, 3, 12, 0, U"LD"_sv, U"BC,d16"_sv, &CPU_detail::ld16_ },
				{ 0x02, 1, 8, 0, U"LD"_sv, U"BC,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x03, 1, 8, 0, U"INC"_sv, U"BC"_sv, &CPU_detail::inc16_ },
				{ 0x04, 1, 4, 0, U"INC"_sv, U"B"_sv, &CPU_detail::inc_ },
				{ 0x05, 1, 4, 0, U"DEC"_sv, U"B"_sv, &CPU_detail::dec_ },
				{ 0x06, 2, 8, 0, U"LD"_sv, U"B,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x07, 1, 4, 0, U"RLCA"_sv, U""_sv, &CPU_detail::rlca_ },
				{ 0x08, 3, 20, 0, U"LD"_sv, U"a16,SP"_sv, &CPU_detail::ld16_n_sp_ },
				{ 0x09, 1, 8, 0, U"ADD"_sv, U"HL,BC"_sv, &CPU_detail::add_hl_ },
				{ 0x0a, 1, 8, 0, U"LD"_sv, U"A,BC"_sv, &CPU_detail::ld_a_n_ },
				{ 0x0b, 1, 8, 0, U"DEC"_sv, U"BC"_sv, &CPU_detail::dec16_ },
				{ 0x0c, 1, 4, 0, U"INC"_sv, U"C"_sv, &CPU_detail::inc_ },
				{ 0x0d, 1, 4, 0, U"DEC"_sv, U"C"_sv, &CPU_detail::dec_ },
				{ 0x0e, 2, 8, 0, U"LD"_sv, U"C,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x0f, 1, 4, 0, U"RRCA"_sv, U""_sv, &CPU_detail::rrca_ },
				{ 0x10, 2, 4, 0, U"STOP"_sv, U"d8"_sv, &CPU_detail::stop_ },
				{ 0x11, 3, 12, 0, U"LD"_sv, U"DE,d16"_sv, &CPU_detail::ld16_ },
				{ 0x12, 1, 8, 0, U"LD"_sv, U"DE,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x13, 1, 8, 0, U"INC"_sv, U"DE"_sv, &CPU_detail::inc16_ },
				{ 0x14, 1, 4, 0, U"INC"_sv, U"D"_sv, &CPU_detail::inc_ },
				{ 0x15, 1, 4, 0, U"DEC"_sv, U"D"_sv, &CPU_detail::dec_ },
				{ 0x16, 2, 8, 0, U"LD"_sv, U"D,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x17, 1, 4, 0, U"RLA"_sv, U""_sv, &CPU_detail::rla_ },
				{ 0x18, 2, 12, 0, U"JR"_sv, U"r8"_sv, &CPU_detail::jr_ },
				{ 0x19, 1, 8, 0, U"ADD"_sv, U"HL,DE"_sv, &CPU_detail::add_hl_ },
				{ 0x1a, 1, 8, 0, U"LD"_sv, U"A,DE"_sv, &CPU_detail::ld_a_n_ },
				{ 0x1b, 1, 8, 0, U"DEC"_sv, U"DE"_sv, &CPU_detail::dec16_ },
				{ 0x1c, 1, 4, 0, U"INC"_sv, U"E"_sv, &CPU_detail::inc_ },
				{ 0x1d, 1, 4, 0, U"DEC"_sv, U"E"_sv, &CPU_detail::dec_ },
				{ 0x1e, 2, 8, 0, U"LD"_sv, U"E,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x1f, 1, 4, 0, U"RRA"_sv, U""_sv, &CPU_detail::rra_ },
				{ 0x20, 2, 12, 8, U"JR"_sv, U"NZ,r8"_sv, &CPU_detail::jr_ },
				{ 0x21, 3, 12, 0, U"LD"_sv, U"HL,d16"_sv, &CPU_detail::ld16_ },
				{ 0x22, 1, 8, 0, U"LDI"_sv, U"HL,A"_sv, &CPU_detail::ldi_hl_a_ },
				{ 0x23, 1, 8, 0, U"INC"_sv, U"HL"_sv, &CPU_detail::inc16_ },
				{ 0x24, 1, 4, 0, U"INC"_sv, U"H"_sv, &CPU_detail::inc_ },
				{ 0x25, 1, 4, 0, U"DEC"_sv, U"H"_sv, &CPU_detail::dec_ },
				{ 0x26, 2, 8, 0, U"LD"_sv, U"H,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x27, 1, 4, 0, U"DAA"_sv, U""_sv, &CPU_detail::daa_ },
				{ 0x28, 2, 12, 8, U"JR"_sv, U"Z,r8"_sv, &CPU_detail::jr_ },
				{ 0x29, 1, 8, 0, U"ADD"_sv, U"HL,HL"_sv, &CPU_detail::add_hl_ },
				{ 0x2a, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, &CPU_detail::ldi_a_hl_ },
				{ 0x2b, 1, 8, 0, U"DEC"_sv, U"HL"_sv, &CPU_detail::dec16_ },
				{ 0x2c, 1, 4, 0, U"INC"_sv, U"L"_sv, &CPU_detail::inc_ },
				{ 0x2d, 1, 4, 0, U"DEC"_sv, U"L"_sv, &CPU_detail::dec_ },
				{ 0x2e, 2, 8, 0, U"LD"_sv, U"L,d8"_sv, &CPU_detail::ld_r_n_ },
				{ 0x2f, 1, 4, 0, U"CPL"_sv, U""_sv, &CPU_detail::cpl_ },
				{ 0x30, 2, 12, 8, U"JR"_sv, U"NC,r8"_sv, &CPU_detail::jr_ },
				{ 0x31, 3, 12, 0, U"LD"_sv, U"SP,d16"_sv, &CPU_detail::ld16_ },
				{ 0x32, 1, 8, 0, U"LD"_sv, U"HL,A"_sv, &CPU_detail::ldd_hl_a_ },
				{ 0x33, 1, 8, 0, U"INC"_sv, U"SP"_sv, &CPU_detail::inc16_ },
				{ 0x34, 1, 12, 0, U"INC"_sv, U"HL"_sv, &CPU_detail::inc_hl_ },
				{ 0x35, 1, 12, 0, U"DEC"_sv, U"HL"_sv, &CPU_detail::dec_hl_ },
				{ 0x36, 2, 12, 0, U"LD"_sv, U"HL,d8"_sv, &CPU_detail::ld_r_r_ },
				{ 0x37, 1, 4, 0, U"SCF"_sv, U""_sv, &CPU_detail::scf_ },
				{ 0x38, 2, 12, 8, U"JR"_sv, U"C,r8"_sv, &CPU_detail::jr_ },
				{ 0x39, 1, 8, 0, U"ADD"_sv, U"HL,SP"_sv, &CPU_detail::add_hl_ },
				{ 0x3a, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, &CPU_detail::ldd_a_hl_ },
				{ 0x3b, 1, 8, 0, U"DEC"_sv, U"SP"_sv, &CPU_detail::dec16_ },
				{ 0x3c, 1, 4, 0, U"INC"_sv, U"A"_sv, &CPU_detail::inc_ },
				{ 0x3d, 1, 4, 0, U"DEC"_sv, U"A"_sv, &CPU_detail::dec_ },
				{ 0x3e, 2, 8, 0, U"LD"_sv, U"A,d8"_sv, &CPU_detail::ld_a_n_ },
				{ 0x3f, 1, 4, 0, U"CCF"_sv, U""_sv, &CPU_detail::ccf_ },
				{ 0x40, 1, 4, 0, U"LD"_sv, U"B,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x41, 1, 4, 0, U"LD"_sv, U"B,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x42, 1, 4, 0, U"LD"_sv, U"B,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x43, 1, 4, 0, U"LD"_sv, U"B,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x44, 1, 4, 0, U"LD"_sv, U"B,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x45, 1, 4, 0, U"LD"_sv, U"B,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x46, 1, 8, 0, U"LD"_sv, U"B,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x47, 1, 4, 0, U"LD"_sv, U"B,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x48, 1, 4, 0, U"LD"_sv, U"C,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x49, 1, 4, 0, U"LD"_sv, U"C,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4a, 1, 4, 0, U"LD"_sv, U"C,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4b, 1, 4, 0, U"LD"_sv, U"C,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4c, 1, 4, 0, U"LD"_sv, U"C,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4d, 1, 4, 0, U"LD"_sv, U"C,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4e, 1, 8, 0, U"LD"_sv, U"C,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x4f, 1, 4, 0, U"LD"_sv, U"C,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x50, 1, 4, 0, U"LD"_sv, U"D,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x51, 1, 4, 0, U"LD"_sv, U"D,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x52, 1, 4, 0, U"LD"_sv, U"D,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x53, 1, 4, 0, U"LD"_sv, U"D,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x54, 1, 4, 0, U"LD"_sv, U"D,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x55, 1, 4, 0, U"LD"_sv, U"D,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x56, 1, 8, 0, U"LD"_sv, U"D,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x57, 1, 4, 0, U"LD"_sv, U"D,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x58, 1, 4, 0, U"LD"_sv, U"E,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x59, 1, 4, 0, U"LD"_sv, U"E,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5a, 1, 4, 0, U"LD"_sv, U"E,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5b, 1, 4, 0, U"LD"_sv, U"E,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5c, 1, 4, 0, U"LD"_sv, U"E,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5d, 1, 4, 0, U"LD"_sv, U"E,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5e, 1, 8, 0, U"LD"_sv, U"E,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x5f, 1, 4, 0, U"LD"_sv, U"E,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x60, 1, 4, 0, U"LD"_sv, U"H,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x61, 1, 4, 0, U"LD"_sv, U"H,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x62, 1, 4, 0, U"LD"_sv, U"H,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x63, 1, 4, 0, U"LD"_sv, U"H,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x64, 1, 4, 0, U"LD"_sv, U"H,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x65, 1, 4, 0, U"LD"_sv, U"H,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x66, 1, 8, 0, U"LD"_sv, U"H,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x67, 1, 4, 0, U"LD"_sv, U"H,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x68, 1, 4, 0, U"LD"_sv, U"L,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x69, 1, 4, 0, U"LD"_sv, U"L,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6a, 1, 4, 0, U"LD"_sv, U"L,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6b, 1, 4, 0, U"LD"_sv, U"L,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6c, 1, 4, 0, U"LD"_sv, U"L,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6d, 1, 4, 0, U"LD"_sv, U"L,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6e, 1, 8, 0, U"LD"_sv, U"L,HL"_sv, &CPU_detail::ld_r_r_ },
				{ 0x6f, 1, 4, 0, U"LD"_sv, U"L,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x70, 1, 8, 0, U"LD"_sv, U"HL,B"_sv, &CPU_detail::ld_r_r_ },
				{ 0x71, 1, 8, 0, U"LD"_sv, U"HL,C"_sv, &CPU_detail::ld_r_r_ },
				{ 0x72, 1, 8, 0, U"LD"_sv, U"HL,D"_sv, &CPU_detail::ld_r_r_ },
				{ 0x73, 1, 8, 0, U"LD"_sv, U"HL,E"_sv, &CPU_detail::ld_r_r_ },
				{ 0x74, 1, 8, 0, U"LD"_sv, U"HL,H"_sv, &CPU_detail::ld_r_r_ },
				{ 0x75, 1, 8, 0, U"LD"_sv, U"HL,L"_sv, &CPU_detail::ld_r_r_ },
				{ 0x76, 1, 4, 0, U"HALT"_sv, U""_sv, &CPU_detail::halt_ },
				{ 0x77, 1, 8, 0, U"LD"_sv, U"HL,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0x78, 1, 4, 0, U"LD"_sv, U"A,B"_sv, &CPU_detail::ld_a_n_ },
				{ 0x79, 1, 4, 0, U"LD"_sv, U"A,C"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7a, 1, 4, 0, U"LD"_sv, U"A,D"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7b, 1, 4, 0, U"LD"_sv, U"A,E"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7c, 1, 4, 0, U"LD"_sv, U"A,H"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7d, 1, 4, 0, U"LD"_sv, U"A,L"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7e, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, &CPU_detail::ld_a_n_ },
				{ 0x7f, 1, 4, 0, U"LD"_sv, U"A,A"_sv, &CPU_detail::ld_a_n_ },
				{ 0x80, 1, 4, 0, U"ADD"_sv, U"A,B"_sv, &CPU_detail::add_a_ },
				{ 0x81, 1, 4, 0, U"ADD"_sv, U"A,C"_sv, &CPU_detail::add_a_ },
				{ 0x82, 1, 4, 0, U"ADD"_sv, U"A,D"_sv, &CPU_detail::add_a_ },
				{ 0x83, 1, 4, 0, U"ADD"_sv, U"A,E"_sv, &CPU_detail::add_a_ },
				{ 0x84, 1, 4, 0, U"ADD"_sv, U"A,H"_sv, &CPU_detail::add_a_ },
				{ 0x85, 1, 4, 0, U"ADD"_sv, U"A,L"_sv, &CPU_detail::add_a_ },
				{ 0x86, 1, 8, 0, U"ADD"_sv, U"A,HL"_sv, &CPU_detail::add_a_ },
				{ 0x87, 1, 4, 0, U"ADD"_sv, U"A,A"_sv, &CPU_detail::add_a_ },
				{ 0x88, 1, 4, 0, U"ADC"_sv, U"A,B"_sv, &CPU_detail::adc_a_ },
				{ 0x89, 1, 4, 0, U"ADC"_sv, U"A,C"_sv, &CPU_detail::adc_a_ },
				{ 0x8a, 1, 4, 0, U"ADC"_sv, U"A,D"_sv, &CPU_detail::adc_a_ },
				{ 0x8b, 1, 4, 0, U"ADC"_sv, U"A,E"_sv, &CPU_detail::adc_a_ },
				{ 0x8c, 1, 4, 0, U"ADC"_sv, U"A,H"_sv, &CPU_detail::adc_a_ },
				{ 0x8d, 1, 4, 0, U"ADC"_sv, U"A,L"_sv, &CPU_detail::adc_a_ },
				{ 0x8e, 1, 8, 0, U"ADC"_sv, U"A,HL"_sv, &CPU_detail::adc_a_ },
				{ 0x8f, 1, 4, 0, U"ADC"_sv, U"A,A"_sv, &CPU_detail::adc_a_ },
				{ 0x90, 1, 4, 0, U"SUB"_sv, U"B"_sv, &CPU_detail::sub_a_ },
				{ 0x91, 1, 4, 0, U"SUB"_sv, U"C"_sv, &CPU_detail::sub_a_ },
				{ 0x92, 1, 4, 0, U"SUB"_sv, U"D"_sv, &CPU_detail::sub_a_ },
				{ 0x93, 1, 4, 0, U"SUB"_sv, U"E"_sv, &CPU_detail::sub_a_ },
				{ 0x94, 1, 4, 0, U"SUB"_sv, U"H"_sv, &CPU_detail::sub_a_ },
				{ 0x95, 1, 4, 0, U"SUB"_sv, U"L"_sv, &CPU_detail::sub_a_ },
				{ 0x96, 1, 8, 0, U"SUB"_sv, U"HL"_sv, &CPU_detail::sub_a_ },
				{ 0x97, 1, 4, 0, U"SUB"_sv, U"A"_sv, &CPU_detail::sub_a_ },
				{ 0x98, 1, 4, 0, U"SBC"_sv, U"A,B"_sv, &CPU_detail::sbc_a_ },
				{ 0x99, 1, 4, 0, U"SBC"_sv, U"A,C"_sv, &CPU_detail::sbc_a_ },
				{ 0x9a, 1, 4, 0, U"SBC"_sv, U"A,D"_sv, &CPU_detail::sbc_a_ },
				{ 0x9b, 1, 4, 0, U"SBC"_sv, U"A,E"_sv, &CPU_detail::sbc_a_ },
				{ 0x9c, 1, 4, 0, U"SBC"_sv, U"A,H"_sv, &CPU_detail::sbc_a_ },
				{ 0x9d, 1, 4, 0, U"SBC"_sv, U"A,L"_sv, &CPU_detail::sbc_a_ },
				{ 0x9e, 1, 8, 0, U"SBC"_sv, U"A,HL"_sv, &CPU_detail::sbc_a_ },
				{ 0x9f, 1, 4, 0, U"SBC"_sv, U"A,A"_sv, &CPU_detail::sbc_a_ },
				{ 0xa0, 1, 4, 0, U"AND"_sv, U"B"_sv, &CPU_detail::and_ },
				{ 0xa1, 1, 4, 0, U"AND"_sv, U"C"_sv, &CPU_detail::and_ },
				{ 0xa2, 1, 4, 0, U"AND"_sv, U"D"_sv, &CPU_detail::and_ },
				{ 0xa3, 1, 4, 0, U"AND"_sv, U"E"_sv, &CPU_detail::and_ },
				{ 0xa4, 1, 4, 0, U"AND"_sv, U"H"_sv, &CPU_detail::and_ },
				{ 0xa5, 1, 4, 0, U"AND"_sv, U"L"_sv, &CPU_detail::and_ },
				{ 0xa6, 1, 8, 0, U"AND"_sv, U"HL"_sv, &CPU_detail::and_ },
				{ 0xa7, 1, 4, 0, U"AND"_sv, U"A"_sv, &CPU_detail::and_ },
				{ 0xa8, 1, 4, 0, U"XOR"_sv, U"B"_sv, &CPU_detail::xor_ },
				{ 0xa9, 1, 4, 0, U"XOR"_sv, U"C"_sv, &CPU_detail::xor_ },
				{ 0xaa, 1, 4, 0, U"XOR"_sv, U"D"_sv, &CPU_detail::xor_ },
				{ 0xab, 1, 4, 0, U"XOR"_sv, U"E"_sv, &CPU_detail::xor_ },
				{ 0xac, 1, 4, 0, U"XOR"_sv, U"H"_sv, &CPU_detail::xor_ },
				{ 0xad, 1, 4, 0, U"XOR"_sv, U"L"_sv, &CPU_detail::xor_ },
				{ 0xae, 1, 8, 0, U"XOR"_sv, U"HL"_sv, &CPU_detail::xor_ },
				{ 0xaf, 1, 4, 0, U"XOR"_sv, U"A"_sv, &CPU_detail::xor_ },
				{ 0xb0, 1, 4, 0, U"OR"_sv, U"B"_sv, &CPU_detail::or_ },
				{ 0xb1, 1, 4, 0, U"OR"_sv, U"C"_sv, &CPU_detail::or_ },
				{ 0xb2, 1, 4, 0, U"OR"_sv, U"D"_sv, &CPU_detail::or_ },
				{ 0xb3, 1, 4, 0, U"OR"_sv, U"E"_sv, &CPU_detail::or_ },
				{ 0xb4, 1, 4, 0, U"OR"_sv, U"H"_sv, &CPU_detail::or_ },
				{ 0xb5, 1, 4, 0, U"OR"_sv, U"L"_sv, &CPU_detail::or_ },
				{ 0xb6, 1, 8, 0, U"OR"_sv, U"HL"_sv, &CPU_detail::or_ },
				{ 0xb7, 1, 4, 0, U"OR"_sv, U"A"_sv, &CPU_detail::or_ },
				{ 0xb8, 1, 4, 0, U"CP"_sv, U"B"_sv, &CPU_detail::cp_ },
				{ 0xb9, 1, 4, 0, U"CP"_sv, U"C"_sv, &CPU_detail::cp_ },
				{ 0xba, 1, 4, 0, U"CP"_sv, U"D"_sv, &CPU_detail::cp_ },
				{ 0xbb, 1, 4, 0, U"CP"_sv, U"E"_sv, &CPU_detail::cp_ },
				{ 0xbc, 1, 4, 0, U"CP"_sv, U"H"_sv, &CPU_detail::cp_ },
				{ 0xbd, 1, 4, 0, U"CP"_sv, U"L"_sv, &CPU_detail::cp_ },
				{ 0xbe, 1, 8, 0, U"CP"_sv, U"HL"_sv, &CPU_detail::cp_ },
				{ 0xbf, 1, 4, 0, U"CP"_sv, U"A"_sv, &CPU_detail::cp_ },
				{ 0xc0, 1, 20, 8, U"RET"_sv, U"NZ"_sv, &CPU_detail::ret_ },
				{ 0xc1, 1, 12, 0, U"POP"_sv, U"BC"_sv, &CPU_detail::pop_ },
				{ 0xc2, 3, 16, 12, U"JP"_sv, U"NZ,a16"_sv, &CPU_detail::jp_ },
				{ 0xc3, 3, 16, 0, U"JP"_sv, U"a16"_sv, &CPU_detail::jp_ },
				{ 0xc4, 3, 24, 12, U"CALL"_sv, U"NZ,a16"_sv, &CPU_detail::call_ },
				{ 0xc5, 1, 16, 0, U"PUSH"_sv, U"BC"_sv, &CPU_detail::push_ },
				{ 0xc6, 2, 8, 0, U"ADD"_sv, U"A,d8"_sv, &CPU_detail::add_a_ },
				{ 0xc7, 1, 16, 0, U"RST"_sv, U"00H"_sv, &CPU_detail::rst_ },
				{ 0xc8, 1, 20, 8, U"RET"_sv, U"Z"_sv, &CPU_detail::ret_ },
				{ 0xc9, 1, 16, 0, U"RET"_sv, U""_sv, &CPU_detail::ret_ },
				{ 0xca, 3, 16, 12, U"JP"_sv, U"Z,a16"_sv, &CPU_detail::jp_ },
				{ 0xcb, 1, 4, 0, U"PREFIX"_sv, U""_sv, &CPU_detail::cbprefix_ },
				{ 0xcc, 3, 24, 12, U"CALL"_sv, U"Z,a16"_sv, &CPU_detail::call_ },
				{ 0xcd, 3, 24, 0, U"CALL"_sv, U"a16"_sv, &CPU_detail::call_ },
				{ 0xce, 2, 8, 0, U"ADC"_sv, U"A,d8"_sv, &CPU_detail::adc_a_ },
				{ 0xcf, 1, 16, 0, U"RST"_sv, U"08H"_sv, &CPU_detail::rst_ },
				{ 0xd0, 1, 20, 8, U"RET"_sv, U"NC"_sv, &CPU_detail::ret_ },
				{ 0xd1, 1, 12, 0, U"POP"_sv, U"DE"_sv, &CPU_detail::pop_ },
				{ 0xd2, 3, 16, 12, U"JP"_sv, U"NC,a16"_sv, &CPU_detail::jp_ },
				{ 0xd3, 1, 4, 0, U"ILLEGAL_D3"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xd4, 3, 24, 12, U"CALL"_sv, U"NC,a16"_sv, &CPU_detail::call_ },
				{ 0xd5, 1, 16, 0, U"PUSH"_sv, U"DE"_sv, &CPU_detail::push_ },
				{ 0xd6, 2, 8, 0, U"SUB"_sv, U"d8"_sv, &CPU_detail::sub_a_ },
				{ 0xd7, 1, 16, 0, U"RST"_sv, U"10H"_sv, &CPU_detail::rst_ },
				{ 0xd8, 1, 20, 8, U"RET"_sv, U"C"_sv, &CPU_detail::ret_ },
				{ 0xd9, 1, 16, 0, U"RETI"_sv, U""_sv, &CPU_detail::reti_ },
				{ 0xda, 3, 16, 12, U"JP"_sv, U"C,a16"_sv, &CPU_detail::jp_ },
				{ 0xdb, 1, 4, 0, U"ILLEGAL_DB"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xdc, 3, 24, 12, U"CALL"_sv, U"C,a16"_sv, &CPU_detail::call_ },
				{ 0xdd, 1, 4, 0, U"ILLEGAL_DD"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xde, 2, 8, 0, U"SBC"_sv, U"A,d8"_sv, &CPU_detail::sbc_a_ },
				{ 0xdf, 1, 16, 0, U"RST"_sv, U"18H"_sv, &CPU_detail::rst_ },
				{ 0xe0, 2, 12, 0, U"LDH"_sv, U"a8,A"_sv, &CPU_detail::ldh_ },
				{ 0xe1, 1, 12, 0, U"POP"_sv, U"HL"_sv, &CPU_detail::pop_ },
				{ 0xe2, 1, 8, 0, U"LD"_sv, U"C,A"_sv, &CPU_detail::ld_c_a_ },
				{ 0xe3, 1, 4, 0, U"ILLEGAL_E3"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xe4, 1, 4, 0, U"ILLEGAL_E4"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xe5, 1, 16, 0, U"PUSH"_sv, U"HL"_sv, &CPU_detail::push_ },
				{ 0xe6, 2, 8, 0, U"AND"_sv, U"d8"_sv, &CPU_detail::and_ },
				{ 0xe7, 1, 16, 0, U"RST"_sv, U"20H"_sv, &CPU_detail::rst_ },
				{ 0xe8, 2, 16, 0, U"ADD"_sv, U"SP,r8"_sv, &CPU_detail::add_sp_ },
				{ 0xe9, 1, 4, 0, U"JP"_sv, U"HL"_sv, &CPU_detail::jp_hl_ },
				{ 0xea, 3, 16, 0, U"LD"_sv, U"a16,A"_sv, &CPU_detail::ld_n_a_ },
				{ 0xeb, 1, 4, 0, U"ILLEGAL_EB"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xec, 1, 4, 0, U"ILLEGAL_EC"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xed, 1, 4, 0, U"ILLEGAL_ED"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xee, 2, 8, 0, U"XOR"_sv, U"d8"_sv, &CPU_detail::xor_ },
				{ 0xef, 1, 16, 0, U"RST"_sv, U"28H"_sv, &CPU_detail::rst_ },
				{ 0xf0, 2, 12, 0, U"LDH"_sv, U"A,a8"_sv, &CPU_detail::ldh_ },
				{ 0xf1, 1, 12, 0, U"POP"_sv, U"AF"_sv, &CPU_detail::pop_ },
				{ 0xf2, 1, 8, 0, U"LD"_sv, U"A,C"_sv, &CPU_detail::ld_a_c_ },
				{ 0xf3, 1, 4, 0, U"DI"_sv, U""_sv, &CPU_detail::di_ },
				{ 0xf4, 1, 4, 0, U"ILLEGAL_F4"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xf5, 1, 16, 0, U"PUSH"_sv, U"AF"_sv, &CPU_detail::push_ },
				{ 0xf6, 2, 8, 0, U"OR"_sv, U"d8"_sv, &CPU_detail::or_ },
				{ 0xf7, 1, 16, 0, U"RST"_sv, U"30H"_sv, &CPU_detail::rst_ },
				{ 0xf8, 2, 12, 0, U"LD"_sv, U"HL,SP,r8"_sv, &CPU_detail::ldhl_sp_n_ },
				{ 0xf9, 1, 8, 0, U"LD"_sv, U"SP,HL"_sv, &CPU_detail::ld16_sp_hl_ },
				{ 0xfa, 3, 16, 0, U"LD"_sv, U"A,a16"_sv, &CPU_detail::ld_a_n_ },
				{ 0xfb, 1, 4, 0, U"EI"_sv, U""_sv, &CPU_detail::ei_ },
				{ 0xfc, 1, 4, 0, U"ILLEGAL_FC"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xfd, 1, 4, 0, U"ILLEGAL_FD"_sv, U""_sv, &CPU_detail::illegal_ },
				{ 0xfe, 2, 8, 0, U"CP"_sv, U"d8"_sv, &CPU_detail::cp_ },
				{ 0xff, 1, 16, 0, U"RST"_sv, U"38H"_sv, &CPU_detail::rst_ },
			}
		};

		// Instructions List (0xcb Prefixed) 0x00-0xff

		std::array<Instruction, 256> cbprefixedInstructions = {
			{
				{ 0x00, 2, 8, 0, U"RLC"_sv, U"B"_sv, &CPU_detail::rlc_ },
				{ 0x01, 2, 8, 0, U"RLC"_sv, U"C"_sv, &CPU_detail::rlc_ },
				{ 0x02, 2, 8, 0, U"RLC"_sv, U"D"_sv, &CPU_detail::rlc_ },
				{ 0x03, 2, 8, 0, U"RLC"_sv, U"E"_sv, &CPU_detail::rlc_ },
				{ 0x04, 2, 8, 0, U"RLC"_sv, U"H"_sv, &CPU_detail::rlc_ },
				{ 0x05, 2, 8, 0, U"RLC"_sv, U"L"_sv, &CPU_detail::rlc_ },
				{ 0x06, 2, 16, 0, U"RLC"_sv, U"HL"_sv, &CPU_detail::rlc_hl_ },
				{ 0x07, 2, 8, 0, U"RLC"_sv, U"A"_sv, &CPU_detail::rlc_ },
				{ 0x08, 2, 8, 0, U"RRC"_sv, U"B"_sv, &CPU_detail::rrc_ },
				{ 0x09, 2, 8, 0, U"RRC"_sv, U"C"_sv, &CPU_detail::rrc_ },
				{ 0x0a, 2, 8, 0, U"RRC"_sv, U"D"_sv, &CPU_detail::rrc_ },
				{ 0x0b, 2, 8, 0, U"RRC"_sv, U"E"_sv, &CPU_detail::rrc_ },
				{ 0x0c, 2, 8, 0, U"RRC"_sv, U"H"_sv, &CPU_detail::rrc_ },
				{ 0x0d, 2, 8, 0, U"RRC"_sv, U"L"_sv, &CPU_detail::rrc_ },
				{ 0x0e, 2, 16, 0, U"RRC"_sv, U"HL"_sv, &CPU_detail::rrc_hl_ },
				{ 0x0f, 2, 8, 0, U"RRC"_sv, U"A"_sv, &CPU_detail::rrc_ },
				{ 0x10, 2, 8, 0, U"RL"_sv, U"B"_sv, &CPU_detail::rl_ },
				{ 0x11, 2, 8, 0, U"RL"_sv, U"C"_sv, &CPU_detail::rl_ },
				{ 0x12, 2, 8, 0, U"RL"_sv, U"D"_sv, &CPU_detail::rl_ },
				{ 0x13, 2, 8, 0, U"RL"_sv, U"E"_sv, &CPU_detail::rl_ },
				{ 0x14, 2, 8, 0, U"RL"_sv, U"H"_sv, &CPU_detail::rl_ },
				{ 0x15, 2, 8, 0, U"RL"_sv, U"L"_sv, &CPU_detail::rl_ },
				{ 0x16, 2, 16, 0, U"RL"_sv, U"HL"_sv, &CPU_detail::rl_hl_ },
				{ 0x17, 2, 8, 0, U"RL"_sv, U"A"_sv, &CPU_detail::rl_ },
				{ 0x18, 2, 8, 0, U"RR"_sv, U"B"_sv, &CPU_detail::rr_ },
				{ 0x19, 2, 8, 0, U"RR"_sv, U"C"_sv, &CPU_detail::rr_ },
				{ 0x1a, 2, 8, 0, U"RR"_sv, U"D"_sv, &CPU_detail::rr_ },
				{ 0x1b, 2, 8, 0, U"RR"_sv, U"E"_sv, &CPU_detail::rr_ },
				{ 0x1c, 2, 8, 0, U"RR"_sv, U"H"_sv, &CPU_detail::rr_ },
				{ 0x1d, 2, 8, 0, U"RR"_sv, U"L"_sv, &CPU_detail::rr_ },
				{ 0x1e, 2, 16, 0, U"RR"_sv, U"HL"_sv, &CPU_detail::rr_hl_ },
				{ 0x1f, 2, 8, 0, U"RR"_sv, U"A"_sv, &CPU_detail::rr_ },
				{ 0x20, 2, 8, 0, U"SLA"_sv, U"B"_sv, &CPU_detail::sla_ },
				{ 0x21, 2, 8, 0, U"SLA"_sv, U"C"_sv, &CPU_detail::sla_ },
				{ 0x22, 2, 8, 0, U"SLA"_sv, U"D"_sv, &CPU_detail::sla_ },
				{ 0x23, 2, 8, 0, U"SLA"_sv, U"E"_sv, &CPU_detail::sla_ },
				{ 0x24, 2, 8, 0, U"SLA"_sv, U"H"_sv, &CPU_detail::sla_ },
				{ 0x25, 2, 8, 0, U"SLA"_sv, U"L"_sv, &CPU_detail::sla_ },
				{ 0x26, 2, 16, 0, U"SLA"_sv, U"HL"_sv, &CPU_detail::sla_hl_ },
				{ 0x27, 2, 8, 0, U"SLA"_sv, U"A"_sv, &CPU_detail::sla_ },
				{ 0x28, 2, 8, 0, U"SRA"_sv, U"B"_sv, &CPU_detail::sra_ },
				{ 0x29, 2, 8, 0, U"SRA"_sv, U"C"_sv, &CPU_detail::sra_ },
				{ 0x2a, 2, 8, 0, U"SRA"_sv, U"D"_sv, &CPU_detail::sra_ },
				{ 0x2b, 2, 8, 0, U"SRA"_sv, U"E"_sv, &CPU_detail::sra_ },
				{ 0x2c, 2, 8, 0, U"SRA"_sv, U"H"_sv, &CPU_detail::sra_ },
				{ 0x2d, 2, 8, 0, U"SRA"_sv, U"L"_sv, &CPU_detail::sra_ },
				{ 0x2e, 2, 16, 0, U"SRA"_sv, U"HL"_sv, &CPU_detail::sra_hl_ },
				{ 0x2f, 2, 8, 0, U"SRA"_sv, U"A"_sv, &CPU_detail::sra_ },
				{ 0x30, 2, 8, 0, U"SWAP"_sv, U"B"_sv, &CPU_detail::swap_ },
				{ 0x31, 2, 8, 0, U"SWAP"_sv, U"C"_sv, &CPU_detail::swap_ },
				{ 0x32, 2, 8, 0, U"SWAP"_sv, U"D"_sv, &CPU_detail::swap_ },
				{ 0x33, 2, 8, 0, U"SWAP"_sv, U"E"_sv, &CPU_detail::swap_ },
				{ 0x34, 2, 8, 0, U"SWAP"_sv, U"H"_sv, &CPU_detail::swap_ },
				{ 0x35, 2, 8, 0, U"SWAP"_sv, U"L"_sv, &CPU_detail::swap_ },
				{ 0x36, 2, 16, 0, U"SWAP"_sv, U"HL"_sv, &CPU_detail::swap_hl_ },
				{ 0x37, 2, 8, 0, U"SWAP"_sv, U"A"_sv, &CPU_detail::swap_ },
				{ 0x38, 2, 8, 0, U"SRL"_sv, U"B"_sv, &CPU_detail::srl_ },
				{ 0x39, 2, 8, 0, U"SRL"_sv, U"C"_sv, &CPU_detail::srl_ },
				{ 0x3a, 2, 8, 0, U"SRL"_sv, U"D"_sv, &CPU_detail::srl_ },
				{ 0x3b, 2, 8, 0, U"SRL"_sv, U"E"_sv, &CPU_detail::srl_ },
				{ 0x3c, 2, 8, 0, U"SRL"_sv, U"H"_sv, &CPU_detail::srl_ },
				{ 0x3d, 2, 8, 0, U"SRL"_sv, U"L"_sv, &CPU_detail::srl_ },
				{ 0x3e, 2, 16, 0, U"SRL"_sv, U"HL"_sv, &CPU_detail::srl_hl_ },
				{ 0x3f, 2, 8, 0, U"SRL"_sv, U"A"_sv, &CPU_detail::srl_ },
				{ 0x40, 2, 8, 0, U"BIT"_sv, U"0,B"_sv, &CPU_detail::bit_ },
				{ 0x41, 2, 8, 0, U"BIT"_sv, U"0,C"_sv, &CPU_detail::bit_ },
				{ 0x42, 2, 8, 0, U"BIT"_sv, U"0,D"_sv, &CPU_detail::bit_ },
				{ 0x43, 2, 8, 0, U"BIT"_sv, U"0,E"_sv, &CPU_detail::bit_ },
				{ 0x44, 2, 8, 0, U"BIT"_sv, U"0,H"_sv, &CPU_detail::bit_ },
				{ 0x45, 2, 8, 0, U"BIT"_sv, U"0,L"_sv, &CPU_detail::bit_ },
				{ 0x46, 2, 12, 0, U"BIT"_sv, U"0,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x47, 2, 8, 0, U"BIT"_sv, U"0,A"_sv, &CPU_detail::bit_ },
				{ 0x48, 2, 8, 0, U"BIT"_sv, U"1,B"_sv, &CPU_detail::bit_ },
				{ 0x49, 2, 8, 0, U"BIT"_sv, U"1,C"_sv, &CPU_detail::bit_ },
				{ 0x4a, 2, 8, 0, U"BIT"_sv, U"1,D"_sv, &CPU_detail::bit_ },
				{ 0x4b, 2, 8, 0, U"BIT"_sv, U"1,E"_sv, &CPU_detail::bit_ },
				{ 0x4c, 2, 8, 0, U"BIT"_sv, U"1,H"_sv, &CPU_detail::bit_ },
				{ 0x4d, 2, 8, 0, U"BIT"_sv, U"1,L"_sv, &CPU_detail::bit_ },
				{ 0x4e, 2, 12, 0, U"BIT"_sv, U"1,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x4f, 2, 8, 0, U"BIT"_sv, U"1,A"_sv, &CPU_detail::bit_ },
				{ 0x50, 2, 8, 0, U"BIT"_sv, U"2,B"_sv, &CPU_detail::bit_ },
				{ 0x51, 2, 8, 0, U"BIT"_sv, U"2,C"_sv, &CPU_detail::bit_ },
				{ 0x52, 2, 8, 0, U"BIT"_sv, U"2,D"_sv, &CPU_detail::bit_ },
				{ 0x53, 2, 8, 0, U"BIT"_sv, U"2,E"_sv, &CPU_detail::bit_ },
				{ 0x54, 2, 8, 0, U"BIT"_sv, U"2,H"_sv, &CPU_detail::bit_ },
				{ 0x55, 2, 8, 0, U"BIT"_sv, U"2,L"_sv, &CPU_detail::bit_ },
				{ 0x56, 2, 12, 0, U"BIT"_sv, U"2,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x57, 2, 8, 0, U"BIT"_sv, U"2,A"_sv, &CPU_detail::bit_ },
				{ 0x58, 2, 8, 0, U"BIT"_sv, U"3,B"_sv, &CPU_detail::bit_ },
				{ 0x59, 2, 8, 0, U"BIT"_sv, U"3,C"_sv, &CPU_detail::bit_ },
				{ 0x5a, 2, 8, 0, U"BIT"_sv, U"3,D"_sv, &CPU_detail::bit_ },
				{ 0x5b, 2, 8, 0, U"BIT"_sv, U"3,E"_sv, &CPU_detail::bit_ },
				{ 0x5c, 2, 8, 0, U"BIT"_sv, U"3,H"_sv, &CPU_detail::bit_ },
				{ 0x5d, 2, 8, 0, U"BIT"_sv, U"3,L"_sv, &CPU_detail::bit_ },
				{ 0x5e, 2, 12, 0, U"BIT"_sv, U"3,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x5f, 2, 8, 0, U"BIT"_sv, U"3,A"_sv, &CPU_detail::bit_ },
				{ 0x60, 2, 8, 0, U"BIT"_sv, U"4,B"_sv, &CPU_detail::bit_ },
				{ 0x61, 2, 8, 0, U"BIT"_sv, U"4,C"_sv, &CPU_detail::bit_ },
				{ 0x62, 2, 8, 0, U"BIT"_sv, U"4,D"_sv, &CPU_detail::bit_ },
				{ 0x63, 2, 8, 0, U"BIT"_sv, U"4,E"_sv, &CPU_detail::bit_ },
				{ 0x64, 2, 8, 0, U"BIT"_sv, U"4,H"_sv, &CPU_detail::bit_ },
				{ 0x65, 2, 8, 0, U"BIT"_sv, U"4,L"_sv, &CPU_detail::bit_ },
				{ 0x66, 2, 12, 0, U"BIT"_sv, U"4,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x67, 2, 8, 0, U"BIT"_sv, U"4,A"_sv, &CPU_detail::bit_ },
				{ 0x68, 2, 8, 0, U"BIT"_sv, U"5,B"_sv, &CPU_detail::bit_ },
				{ 0x69, 2, 8, 0, U"BIT"_sv, U"5,C"_sv, &CPU_detail::bit_ },
				{ 0x6a, 2, 8, 0, U"BIT"_sv, U"5,D"_sv, &CPU_detail::bit_ },
				{ 0x6b, 2, 8, 0, U"BIT"_sv, U"5,E"_sv, &CPU_detail::bit_ },
				{ 0x6c, 2, 8, 0, U"BIT"_sv, U"5,H"_sv, &CPU_detail::bit_ },
				{ 0x6d, 2, 8, 0, U"BIT"_sv, U"5,L"_sv, &CPU_detail::bit_ },
				{ 0x6e, 2, 12, 0, U"BIT"_sv, U"5,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x6f, 2, 8, 0, U"BIT"_sv, U"5,A"_sv, &CPU_detail::bit_ },
				{ 0x70, 2, 8, 0, U"BIT"_sv, U"6,B"_sv, &CPU_detail::bit_ },
				{ 0x71, 2, 8, 0, U"BIT"_sv, U"6,C"_sv, &CPU_detail::bit_ },
				{ 0x72, 2, 8, 0, U"BIT"_sv, U"6,D"_sv, &CPU_detail::bit_ },
				{ 0x73, 2, 8, 0, U"BIT"_sv, U"6,E"_sv, &CPU_detail::bit_ },
				{ 0x74, 2, 8, 0, U"BIT"_sv, U"6,H"_sv, &CPU_detail::bit_ },
				{ 0x75, 2, 8, 0, U"BIT"_sv, U"6,L"_sv, &CPU_detail::bit_ },
				{ 0x76, 2, 12, 0, U"BIT"_sv, U"6,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x77, 2, 8, 0, U"BIT"_sv, U"6,A"_sv, &CPU_detail::bit_ },
				{ 0x78, 2, 8, 0, U"BIT"_sv, U"7,B"_sv, &CPU_detail::bit_ },
				{ 0x79, 2, 8, 0, U"BIT"_sv, U"7,C"_sv, &CPU_detail::bit_ },
				{ 0x7a, 2, 8, 0, U"BIT"_sv, U"7,D"_sv, &CPU_detail::bit_ },
				{ 0x7b, 2, 8, 0, U"BIT"_sv, U"7,E"_sv, &CPU_detail::bit_ },
				{ 0x7c, 2, 8, 0, U"BIT"_sv, U"7,H"_sv, &CPU_detail::bit_ },
				{ 0x7d, 2, 8, 0, U"BIT"_sv, U"7,L"_sv, &CPU_detail::bit_ },
				{ 0x7e, 2, 12, 0, U"BIT"_sv, U"7,HL"_sv, &CPU_detail::bit_hl_ },
				{ 0x7f, 2, 8, 0, U"BIT"_sv, U"7,A"_sv, &CPU_detail::bit_ },
				{ 0x80, 2, 8, 0, U"RES"_sv, U"0,B"_sv, &CPU_detail::res_ },
				{ 0x81, 2, 8, 0, U"RES"_sv, U"0,C"_sv, &CPU_detail::res_ },
				{ 0x82, 2, 8, 0, U"RES"_sv, U"0,D"_sv, &CPU_detail::res_ },
				{ 0x83, 2, 8, 0, U"RES"_sv, U"0,E"_sv, &CPU_detail::res_ },
				{ 0x84, 2, 8, 0, U"RES"_sv, U"0,H"_sv, &CPU_detail::res_ },
				{ 0x85, 2, 8, 0, U"RES"_sv, U"0,L"_sv, &CPU_detail::res_ },
				{ 0x86, 2, 16, 0, U"RES"_sv, U"0,HL"_sv, &CPU_detail::res_hl_ },
				{ 0x87, 2, 8, 0, U"RES"_sv, U"0,A"_sv, &CPU_detail::res_ },
				{ 0x88, 2, 8, 0, U"RES"_sv, U"1,B"_sv, &CPU_detail::res_ },
				{ 0x89, 2, 8, 0, U"RES"_sv, U"1,C"_sv, &CPU_detail::res_ },
				{ 0x8a, 2, 8, 0, U"RES"_sv, U"1,D"_sv, &CPU_detail::res_ },
				{ 0x8b, 2, 8, 0, U"RES"_sv, U"1,E"_sv, &CPU_detail::res_ },
				{ 0x8c, 2, 8, 0, U"RES"_sv, U"1,H"_sv, &CPU_detail::res_ },
				{ 0x8d, 2, 8, 0, U"RES"_sv, U"1,L"_sv, &CPU_detail::res_ },
				{ 0x8e, 2, 16, 0, U"RES"_sv, U"1,HL"_sv, &CPU_detail::res_hl_ },
				{ 0x8f, 2, 8, 0, U"RES"_sv, U"1,A"_sv, &CPU_detail::res_ },
				{ 0x90, 2, 8, 0, U"RES"_sv, U"2,B"_sv, &CPU_detail::res_ },
				{ 0x91, 2, 8, 0, U"RES"_sv, U"2,C"_sv, &CPU_detail::res_ },
				{ 0x92, 2, 8, 0, U"RES"_sv, U"2,D"_sv, &CPU_detail::res_ },
				{ 0x93, 2, 8, 0, U"RES"_sv, U"2,E"_sv, &CPU_detail::res_ },
				{ 0x94, 2, 8, 0, U"RES"_sv, U"2,H"_sv, &CPU_detail::res_ },
				{ 0x95, 2, 8, 0, U"RES"_sv, U"2,L"_sv, &CPU_detail::res_ },
				{ 0x96, 2, 16, 0, U"RES"_sv, U"2,HL"_sv, &CPU_detail::res_hl_ },
				{ 0x97, 2, 8, 0, U"RES"_sv, U"2,A"_sv, &CPU_detail::res_ },
				{ 0x98, 2, 8, 0, U"RES"_sv, U"3,B"_sv, &CPU_detail::res_ },
				{ 0x99, 2, 8, 0, U"RES"_sv, U"3,C"_sv, &CPU_detail::res_ },
				{ 0x9a, 2, 8, 0, U"RES"_sv, U"3,D"_sv, &CPU_detail::res_ },
				{ 0x9b, 2, 8, 0, U"RES"_sv, U"3,E"_sv, &CPU_detail::res_ },
				{ 0x9c, 2, 8, 0, U"RES"_sv, U"3,H"_sv, &CPU_detail::res_ },
				{ 0x9d, 2, 8, 0, U"RES"_sv, U"3,L"_sv, &CPU_detail::res_ },
				{ 0x9e, 2, 16, 0, U"RES"_sv, U"3,HL"_sv, &CPU_detail::res_hl_ },
				{ 0x9f, 2, 8, 0, U"RES"_sv, U"3,A"_sv, &CPU_detail::res_ },
				{ 0xa0, 2, 8, 0, U"RES"_sv, U"4,B"_sv, &CPU_detail::res_ },
				{ 0xa1, 2, 8, 0, U"RES"_sv, U"4,C"_sv, &CPU_detail::res_ },
				{ 0xa2, 2, 8, 0, U"RES"_sv, U"4,D"_sv, &CPU_detail::res_ },
				{ 0xa3, 2, 8, 0, U"RES"_sv, U"4,E"_sv, &CPU_detail::res_ },
				{ 0xa4, 2, 8, 0, U"RES"_sv, U"4,H"_sv, &CPU_detail::res_ },
				{ 0xa5, 2, 8, 0, U"RES"_sv, U"4,L"_sv, &CPU_detail::res_ },
				{ 0xa6, 2, 16, 0, U"RES"_sv, U"4,HL"_sv, &CPU_detail::res_hl_ },
				{ 0xa7, 2, 8, 0, U"RES"_sv, U"4,A"_sv, &CPU_detail::res_ },
				{ 0xa8, 2, 8, 0, U"RES"_sv, U"5,B"_sv, &CPU_detail::res_ },
				{ 0xa9, 2, 8, 0, U"RES"_sv, U"5,C"_sv, &CPU_detail::res_ },
				{ 0xaa, 2, 8, 0, U"RES"_sv, U"5,D"_sv, &CPU_detail::res_ },
				{ 0xab, 2, 8, 0, U"RES"_sv, U"5,E"_sv, &CPU_detail::res_ },
				{ 0xac, 2, 8, 0, U"RES"_sv, U"5,H"_sv, &CPU_detail::res_ },
				{ 0xad, 2, 8, 0, U"RES"_sv, U"5,L"_sv, &CPU_detail::res_ },
				{ 0xae, 2, 16, 0, U"RES"_sv, U"5,HL"_sv, &CPU_detail::res_hl_ },
				{ 0xaf, 2, 8, 0, U"RES"_sv, U"5,A"_sv, &CPU_detail::res_ },
				{ 0xb0, 2, 8, 0, U"RES"_sv, U"6,B"_sv, &CPU_detail::res_ },
				{ 0xb1, 2, 8, 0, U"RES"_sv, U"6,C"_sv, &CPU_detail::res_ },
				{ 0xb2, 2, 8, 0, U"RES"_sv, U"6,D"_sv, &CPU_detail::res_ },
				{ 0xb3, 2, 8, 0, U"RES"_sv, U"6,E"_sv, &CPU_detail::res_ },
				{ 0xb4, 2, 8, 0, U"RES"_sv, U"6,H"_sv, &CPU_detail::res_ },
				{ 0xb5, 2, 8, 0, U"RES"_sv, U"6,L"_sv, &CPU_detail::res_ },
				{ 0xb6, 2, 16, 0, U"RES"_sv, U"6,HL"_sv, &CPU_detail::res_hl_ },
				{ 0xb7, 2, 8, 0, U"RES"_sv, U"6,A"_sv, &CPU_detail::res_ },
				{ 0xb8, 2, 8, 0, U"RES"_sv, U"7,B"_sv, &CPU_detail::res_ },
				{ 0xb9, 2, 8, 0, U"RES"_sv, U"7,C"_sv, &CPU_detail::res_ },
				{ 0xba, 2, 8, 0, U"RES"_sv, U"7,D"_sv, &CPU_detail::res_ },
				{ 0xbb, 2, 8, 0, U"RES"_sv, U"7,E"_sv, &CPU_detail::res_ },
				{ 0xbc, 2, 8, 0, U"RES"_sv, U"7,H"_sv, &CPU_detail::res_ },
				{ 0xbd, 2, 8, 0, U"RES"_sv, U"7,L"_sv, &CPU_detail::res_ },
				{ 0xbe, 2, 16, 0, U"RES"_sv, U"7,HL"_sv, &CPU_detail::res_hl_ },
				{ 0xbf, 2, 8, 0, U"RES"_sv, U"7,A"_sv, &CPU_detail::res_ },
				{ 0xc0, 2, 8, 0, U"SET"_sv, U"0,B"_sv, &CPU_detail::set_ },
				{ 0xc1, 2, 8, 0, U"SET"_sv, U"0,C"_sv, &CPU_detail::set_ },
				{ 0xc2, 2, 8, 0, U"SET"_sv, U"0,D"_sv, &CPU_detail::set_ },
				{ 0xc3, 2, 8, 0, U"SET"_sv, U"0,E"_sv, &CPU_detail::set_ },
				{ 0xc4, 2, 8, 0, U"SET"_sv, U"0,H"_sv, &CPU_detail::set_ },
				{ 0xc5, 2, 8, 0, U"SET"_sv, U"0,L"_sv, &CPU_detail::set_ },
				{ 0xc6, 2, 16, 0, U"SET"_sv, U"0,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xc7, 2, 8, 0, U"SET"_sv, U"0,A"_sv, &CPU_detail::set_ },
				{ 0xc8, 2, 8, 0, U"SET"_sv, U"1,B"_sv, &CPU_detail::set_ },
				{ 0xc9, 2, 8, 0, U"SET"_sv, U"1,C"_sv, &CPU_detail::set_ },
				{ 0xca, 2, 8, 0, U"SET"_sv, U"1,D"_sv, &CPU_detail::set_ },
				{ 0xcb, 2, 8, 0, U"SET"_sv, U"1,E"_sv, &CPU_detail::set_ },
				{ 0xcc, 2, 8, 0, U"SET"_sv, U"1,H"_sv, &CPU_detail::set_ },
				{ 0xcd, 2, 8, 0, U"SET"_sv, U"1,L"_sv, &CPU_detail::set_ },
				{ 0xce, 2, 16, 0, U"SET"_sv, U"1,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xcf, 2, 8, 0, U"SET"_sv, U"1,A"_sv, &CPU_detail::set_ },
				{ 0xd0, 2, 8, 0, U"SET"_sv, U"2,B"_sv, &CPU_detail::set_ },
				{ 0xd1, 2, 8, 0, U"SET"_sv, U"2,C"_sv, &CPU_detail::set_ },
				{ 0xd2, 2, 8, 0, U"SET"_sv, U"2,D"_sv, &CPU_detail::set_ },
				{ 0xd3, 2, 8, 0, U"SET"_sv, U"2,E"_sv, &CPU_detail::set_ },
				{ 0xd4, 2, 8, 0, U"SET"_sv, U"2,H"_sv, &CPU_detail::set_ },
				{ 0xd5, 2, 8, 0, U"SET"_sv, U"2,L"_sv, &CPU_detail::set_ },
				{ 0xd6, 2, 16, 0, U"SET"_sv, U"2,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xd7, 2, 8, 0, U"SET"_sv, U"2,A"_sv, &CPU_detail::set_ },
				{ 0xd8, 2, 8, 0, U"SET"_sv, U"3,B"_sv, &CPU_detail::set_ },
				{ 0xd9, 2, 8, 0, U"SET"_sv, U"3,C"_sv, &CPU_detail::set_ },
				{ 0xda, 2, 8, 0, U"SET"_sv, U"3,D"_sv, &CPU_detail::set_ },
				{ 0xdb, 2, 8, 0, U"SET"_sv, U"3,E"_sv, &CPU_detail::set_ },
				{ 0xdc, 2, 8, 0, U"SET"_sv, U"3,H"_sv, &CPU_detail::set_ },
				{ 0xdd, 2, 8, 0, U"SET"_sv, U"3,L"_sv, &CPU_detail::set_ },
				{ 0xde, 2, 16, 0, U"SET"_sv, U"3,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xdf, 2, 8, 0, U"SET"_sv, U"3,A"_sv, &CPU_detail::set_ },
				{ 0xe0, 2, 8, 0, U"SET"_sv, U"4,B"_sv, &CPU_detail::set_ },
				{ 0xe1, 2, 8, 0, U"SET"_sv, U"4,C"_sv, &CPU_detail::set_ },
				{ 0xe2, 2, 8, 0, U"SET"_sv, U"4,D"_sv, &CPU_detail::set_ },
				{ 0xe3, 2, 8, 0, U"SET"_sv, U"4,E"_sv, &CPU_detail::set_ },
				{ 0xe4, 2, 8, 0, U"SET"_sv, U"4,H"_sv, &CPU_detail::set_ },
				{ 0xe5, 2, 8, 0, U"SET"_sv, U"4,L"_sv, &CPU_detail::set_ },
				{ 0xe6, 2, 16, 0, U"SET"_sv, U"4,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xe7, 2, 8, 0, U"SET"_sv, U"4,A"_sv, &CPU_detail::set_ },
				{ 0xe8, 2, 8, 0, U"SET"_sv, U"5,B"_sv, &CPU_detail::set_ },
				{ 0xe9, 2, 8, 0, U"SET"_sv, U"5,C"_sv, &CPU_detail::set_ },
				{ 0xea, 2, 8, 0, U"SET"_sv, U"5,D"_sv, &CPU_detail::set_ },
				{ 0xeb, 2, 8, 0, U"SET"_sv, U"5,E"_sv, &CPU_detail::set_ },
				{ 0xec, 2, 8, 0, U"SET"_sv, U"5,H"_sv, &CPU_detail::set_ },
				{ 0xed, 2, 8, 0, U"SET"_sv, U"5,L"_sv, &CPU_detail::set_ },
				{ 0xee, 2, 16, 0, U"SET"_sv, U"5,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xef, 2, 8, 0, U"SET"_sv, U"5,A"_sv, &CPU_detail::set_ },
				{ 0xf0, 2, 8, 0, U"SET"_sv, U"6,B"_sv, &CPU_detail::set_ },
				{ 0xf1, 2, 8, 0, U"SET"_sv, U"6,C"_sv, &CPU_detail::set_ },
				{ 0xf2, 2, 8, 0, U"SET"_sv, U"6,D"_sv, &CPU_detail::set_ },
				{ 0xf3, 2, 8, 0, U"SET"_sv, U"6,E"_sv, &CPU_detail::set_ },
				{ 0xf4, 2, 8, 0, U"SET"_sv, U"6,H"_sv, &CPU_detail::set_ },
				{ 0xf5, 2, 8, 0, U"SET"_sv, U"6,L"_sv, &CPU_detail::set_ },
				{ 0xf6, 2, 16, 0, U"SET"_sv, U"6,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xf7, 2, 8, 0, U"SET"_sv, U"6,A"_sv, &CPU_detail::set_ },
				{ 0xf8, 2, 8, 0, U"SET"_sv, U"7,B"_sv, &CPU_detail::set_ },
				{ 0xf9, 2, 8, 0, U"SET"_sv, U"7,C"_sv, &CPU_detail::set_ },
				{ 0xfa, 2, 8, 0, U"SET"_sv, U"7,D"_sv, &CPU_detail::set_ },
				{ 0xfb, 2, 8, 0, U"SET"_sv, U"7,E"_sv, &CPU_detail::set_ },
				{ 0xfc, 2, 8, 0, U"SET"_sv, U"7,H"_sv, &CPU_detail::set_ },
				{ 0xfd, 2, 8, 0, U"SET"_sv, U"7,L"_sv, &CPU_detail::set_ },
				{ 0xfe, 2, 16, 0, U"SET"_sv, U"7,HL"_sv, &CPU_detail::set_hl_ },
				{ 0xff, 2, 8, 0, U"SET"_sv, U"7,A"_sv, &CPU_detail::set_ },
			}
		};
	};


	CPU::CPU(Memory* mem, Interrupt* interrupt)
		: mem_{ mem }, cpuDetail_{ std::make_unique<CPU_detail>(mem, interrupt) }
	{
		cpuDetail_->reset();
	}

	CPU::~CPU()
	{
	}

	void CPU::setCGBMode(bool value)
	{
		cpuDetail_->setCGBMode(value);
	}

	void CPU::reset()
	{
		cpuDetail_->reset();
	}

	void CPU::run()
	{
		cpuDetail_->run();
	}

	void CPU::interrupt()
	{
		cpuDetail_->interrupt();
	}

	//void CPU::applyScheduledIME()
	//{
	//	cpuDetail_->applyScheduledIME();
	//}

	void CPU::dump()
	{
		cpuDetail_->dump();
	}

	uint16 CPU::currentPC() const
	{
		return cpuDetail_->pc;
	}

	int CPU::consumedCycles() const
	{
		return cpuDetail_->consumedCycles_;
	}

	RegisterValues CPU::getRegisterValues() const
	{
		return {
			cpuDetail_->af(),
			cpuDetail_->bc(),
			cpuDetail_->de(),
			cpuDetail_->hl(),
			cpuDetail_->sp,
			cpuDetail_->pc,
		};
	}
}
