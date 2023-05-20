#include "CPU.h"
#include "Memory.h"
#include "PPU.h"
#include "LCD.h"
#include "Timer.h"

namespace dmge
{
	namespace Instructions
	{
		//fwd

		void ld_r_n_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld_r_r_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld_a_n_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld_n_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld_a_c_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld_c_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ldd_a_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ldd_hl_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ldi_a_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ldi_hl_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ldh_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld16_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld16_sp_hl_(CPU* cpu, Memory*, const Instruction*);
		void ldhl_sp_n_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ld16_n_sp_(CPU* cpu, Memory* mem, const Instruction* inst);
		void push_(CPU* cpu, Memory* mem, const Instruction* inst);
		void pop_(CPU* cpu, Memory* mem, const Instruction* inst);

		void add_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void adc_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sub_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sbc_a_(CPU* cpu, Memory* mem, const Instruction* inst);
		void and_(CPU* cpu, Memory* mem, const Instruction* inst);
		void or_(CPU* cpu, Memory* mem, const Instruction* inst);
		void xor_(CPU* cpu, Memory* mem, const Instruction* inst);
		void cp_(CPU* cpu, Memory* mem, const Instruction* inst);
		void inc_(CPU* cpu, Memory* mem, const Instruction* inst);
		void inc_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void dec_(CPU* cpu, Memory* mem, const Instruction* inst);
		void dec_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void add_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void add_sp_(CPU* cpu, Memory* mem, const Instruction* inst);
		void inc16_(CPU* cpu, Memory* mem, const Instruction* inst);
		void dec16_(CPU* cpu, Memory* mem, const Instruction* inst);

		void swap_(CPU* cpu, Memory* mem, const Instruction* inst);
		void swap_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void daa_(CPU* cpu, Memory* mem, const Instruction* inst);
		void cpl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void ccf_(CPU* cpu, Memory* mem, const Instruction* inst);
		void scf_(CPU* cpu, Memory* mem, const Instruction* inst);
		void nop_(CPU*, Memory*, const Instruction*);
		void halt_(CPU*, Memory*, const Instruction*);
		void stop_(CPU*, Memory*, const Instruction*);
		void di_(CPU*, Memory*, const Instruction*);
		void ei_(CPU*, Memory*, const Instruction*);

		void rlca_(CPU*, Memory*, const Instruction*);
		void rla_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rrca_(CPU*, Memory*, const Instruction*);
		void rra_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rlc_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rlc_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rl_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rrc_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rrc_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rr_(CPU* cpu, Memory* mem, const Instruction* inst);
		void rr_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sla_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sla_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sra_(CPU* cpu, Memory* mem, const Instruction* inst);
		void sra_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void srl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void srl_hl_(CPU* cpu, Memory* mem, const Instruction* inst);

		void bit_(CPU* cpu, Memory* mem, const Instruction* inst);
		void bit_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void set_(CPU* cpu, Memory* mem, const Instruction* inst);
		void set_hl_(CPU* cpu, Memory* mem, const Instruction* inst);
		void res_(CPU* cpu, Memory* mem, const Instruction* inst);
		void res_hl_(CPU* cpu, Memory* mem, const Instruction* inst);

		void jp_(CPU* cpu, Memory* mem, const Instruction* inst);
		void jp_hl_(CPU* cpu, Memory*, const Instruction*);
		void jr_(CPU* cpu, Memory* mem, const Instruction* inst);

		void call_(CPU* cpu, Memory* mem, const Instruction* inst);

		void rst_(CPU* cpu, Memory* mem, const Instruction* inst);

		void ret_(CPU* cpu, Memory* mem, const Instruction* inst);
		void reti_(CPU* cpu, Memory* mem, const Instruction* inst);

		void cbprefix_(CPU* cpu, Memory* mem, const Instruction* inst);
		void illegal_(CPU* cpu, Memory* mem, const Instruction* inst);

		// Loads
		// (LD, LDI, LDD, LDH, PUSH, POP)

		void ld_r_n_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint8 n = mem->read(cpu->pc + 1);

			switch (inst->opcode)
			{
			case 0x06: cpu->b = n; return;
			case 0x0e: cpu->c = n; return;
			case 0x16: cpu->d = n; return;
			case 0x1e: cpu->e = n; return;
			case 0x26: cpu->h = n; return;
			case 0x2e: cpu->l = n; return;
			default: return;
			}
		}

		void ld_r_r_(CPU* cpu, Memory* mem, const Instruction* inst)
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
			case 0x40: cpu->b = cpu->b; return;
			case 0x41: cpu->b = cpu->c; return;
			case 0x42: cpu->b = cpu->d; return;
			case 0x43: cpu->b = cpu->e; return;
			case 0x44: cpu->b = cpu->h; return;
			case 0x45: cpu->b = cpu->l; return;
			case 0x46: cpu->b = mem->read(cpu->hl()); return;

			// C
			case 0x48: cpu->c = cpu->b; return;
			case 0x49: cpu->c = cpu->c; return;
			case 0x4a: cpu->c = cpu->d; return;
			case 0x4b: cpu->c = cpu->e; return;
			case 0x4c: cpu->c = cpu->h; return;
			case 0x4d: cpu->c = cpu->l; return;
			case 0x4e: cpu->c = mem->read(cpu->hl()); return;

			// D
			case 0x50: cpu->d = cpu->b; return;
			case 0x51: cpu->d = cpu->c; return;
			case 0x52: cpu->d = cpu->d; return;
			case 0x53: cpu->d = cpu->e; return;
			case 0x54: cpu->d = cpu->h; return;
			case 0x55: cpu->d = cpu->l; return;
			case 0x56: cpu->d = mem->read(cpu->hl()); return;

			// E
			case 0x58: cpu->e = cpu->b; return;
			case 0x59: cpu->e = cpu->c; return;
			case 0x5a: cpu->e = cpu->d; return;
			case 0x5b: cpu->e = cpu->e; return;
			case 0x5c: cpu->e = cpu->h; return;
			case 0x5d: cpu->e = cpu->l; return;
			case 0x5e: cpu->e = mem->read(cpu->hl()); return;

			//H
			case 0x60: cpu->h = cpu->b; return;
			case 0x61: cpu->h = cpu->c; return;
			case 0x62: cpu->h = cpu->d; return;
			case 0x63: cpu->h = cpu->e; return;
			case 0x64: cpu->h = cpu->h; return;
			case 0x65: cpu->h = cpu->l; return;
			case 0x66: cpu->h = mem->read(cpu->hl()); return;

			// L
			case 0x68: cpu->l = cpu->b; return;
			case 0x69: cpu->l = cpu->c; return;
			case 0x6a: cpu->l = cpu->d; return;
			case 0x6b: cpu->l = cpu->e; return;
			case 0x6c: cpu->l = cpu->h; return;
			case 0x6d: cpu->l = cpu->l; return;
			case 0x6e: cpu->l = mem->read(cpu->hl()); return;

			// (HL)
			case 0x70: mem->write(cpu->hl(), cpu->b); return;
			case 0x71: mem->write(cpu->hl(), cpu->c); return;
			case 0x72: mem->write(cpu->hl(), cpu->d); return;
			case 0x73: mem->write(cpu->hl(), cpu->e); return;
			case 0x74: mem->write(cpu->hl(), cpu->h); return;
			case 0x75: mem->write(cpu->hl(), cpu->l); return;
			case 0x36: mem->write(cpu->hl(), mem->read(cpu->pc + 1)); return;

			default: return;
			}
		}

		void ld_a_n_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0x7f: return;
			case 0x78: n = cpu->b; break;
			case 0x79: n = cpu->c; break;
			case 0x7a: n = cpu->d; break;
			case 0x7b: n = cpu->e; break;
			case 0x7c: n = cpu->h; break;
			case 0x7d: n = cpu->l; break;
			case 0x0a: n = mem->read(cpu->bc()); break;
			case 0x1a: n = mem->read(cpu->de()); break;
			case 0x7e: n = mem->read(cpu->hl()); break;
			case 0xfa: n = mem->read(mem->read16(cpu->pc + 1)); break;
			case 0x3e: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->a = n;
		}

		void ld_n_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x7f: cpu->a = cpu->a; return;
			case 0x47: cpu->b = cpu->a; return;
			case 0x4f: cpu->c = cpu->a; return;
			case 0x57: cpu->d = cpu->a; return;
			case 0x5f: cpu->e = cpu->a; return;
			case 0x67: cpu->h = cpu->a; return;
			case 0x6f: cpu->l = cpu->a; return;
			case 0x02: mem->write(cpu->bc(), cpu->a); return;
			case 0x12: mem->write(cpu->de(), cpu->a); return;
			case 0x77: mem->write(cpu->hl(), cpu->a); return;
			case 0xea: mem->write(mem->read16(cpu->pc + 1), cpu->a); return;
			default: return;
			}
		}

		void ld_a_c_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0xf2
			cpu->a = mem->read(0xff00 + cpu->c);
		}

		void ld_c_a_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0xe2
			mem->write(0xff00 + cpu->c, cpu->a);
		}

		void ldd_a_hl_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			// opcode: 0x3a
			cpu->a = mem->read(cpu->hl());
			dec16_(cpu, mem, inst);
		}

		void ldd_hl_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			// opcode: 0x32
			mem->write(cpu->hl(), cpu->a);
			dec16_(cpu, mem, inst);
		}

		void ldi_a_hl_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			// opcode: 0x2a
			cpu->a = mem->read(cpu->hl());
			inc16_(cpu, mem, inst);
		}

		void ldi_hl_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			// opcode: 0x22
			mem->write(cpu->hl(), cpu->a);
			inc16_(cpu, mem, inst);
		}

		void ldh_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint8 n = mem->read(cpu->pc + 1);

			switch (inst->opcode)
			{
			case 0xe0:
				mem->write(0xff00 + n, cpu->a);
				return;
			case 0xf0:
				cpu->a = mem->read(0xff00 + n);
				return;
			default:
				return;
			}
		}

		void ld16_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint16 n = mem->read16(cpu->pc + 1);

			switch (inst->opcode)
			{
			case 0x01: cpu->bc(n); return;
			case 0x11: cpu->de(n); return;
			case 0x21: cpu->hl(n); return;
			case 0x31: cpu->sp = n; return;
			default: return;
			}
		}

		void ld16_sp_hl_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0xf9
			cpu->sp = cpu->hl();
		}

		void ldhl_sp_n_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0xf8

			const int8 n = mem->read(cpu->pc + 1);

			cpu->hl(cpu->sp + n);  //※符号付演算

			cpu->f_z(false);
			cpu->f_n(false);
			cpu->f_h((cpu->sp & 0xf) + (n & 0xf) > 0xf);
			cpu->f_c((cpu->sp & 0xff) + (n & 0xff) > 0xff);
		}

		void ld16_n_sp_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x08
			const uint16 addr = mem->read16(cpu->pc + 1);
			mem->write(addr, cpu->sp & 0xff);
			mem->write(addr + 1, (cpu->sp >> 8) & 0xff);
		}

		void push_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0xf5:
				mem->write(--cpu->sp, cpu->a);
				mem->write(--cpu->sp, cpu->f);
				return;

			case 0xc5:
				mem->write(--cpu->sp, cpu->b);
				mem->write(--cpu->sp, cpu->c);
				return;

			case 0xd5:
				mem->write(--cpu->sp, cpu->d);
				mem->write(--cpu->sp, cpu->e);
				return;

			case 0xe5:
				mem->write(--cpu->sp, cpu->h);
				mem->write(--cpu->sp, cpu->l);
				return;

			default: return;
			}
		}

		void pop_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0xf1:
				cpu->af(mem->read16(cpu->sp++) & 0xfff0);
				break;

			case 0xc1:
				cpu->bc(mem->read16(cpu->sp++));
				break;

			case 0xd1:
				cpu->de(mem->read16(cpu->sp++));
				break;

			case 0xe1:
				cpu->hl(mem->read16(cpu->sp++));
				break;
			}

			++cpu->sp;
		}

		// Arithmetic
		// (ADD, ADC, SUB, SBC, AND, OR, XOR, CP, INC, DEC)

		void add_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x87: n = cpu->a; break;
			case 0x80: n = cpu->b; break;
			case 0x81: n = cpu->c; break;
			case 0x82: n = cpu->d; break;
			case 0x83: n = cpu->e; break;
			case 0x84: n = cpu->h; break;
			case 0x85: n = cpu->l; break;
			case 0x86: n = mem->read(cpu->hl()); break;
			case 0xc6: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->f_n(false);
			cpu->f_h((cpu->a & 0xf) + (n & 0xf) > 0xf);
			cpu->f_c((cpu->a & 0xff) + (n & 0xff) > 0xff);
			cpu->a += n;
			cpu->f_z(cpu->a == 0);
		}

		void adc_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x8f: n = cpu->a; break;
			case 0x88: n = cpu->b; break;
			case 0x89: n = cpu->c; break;
			case 0x8a: n = cpu->d; break;
			case 0x8b: n = cpu->e; break;
			case 0x8c: n = cpu->h; break;
			case 0x8d: n = cpu->l; break;
			case 0x8e: n = mem->read(cpu->hl()); break;
			case 0xce: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			const uint8 c = (uint8)cpu->f_c();

			cpu->f_n(false);
			cpu->f_h((cpu->a & 0xf) + (n & 0xf) + c > 0xf);
			cpu->f_c((cpu->a & 0xff) + (n & 0xff) + c > 0xff);
			cpu->a += n + c;
			cpu->f_z(cpu->a == 0);
		}

		void sub_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x97: n = cpu->a; break;
			case 0x90: n = cpu->b; break;
			case 0x91: n = cpu->c; break;
			case 0x92: n = cpu->d; break;
			case 0x93: n = cpu->e; break;
			case 0x94: n = cpu->h; break;
			case 0x95: n = cpu->l; break;
			case 0x96: n = mem->read(cpu->hl()); break;
			case 0xd6: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->f_n(true);
			cpu->f_h((cpu->a & 0xf) - (n & 0xf) < 0);
			cpu->f_c(cpu->a < n);
			cpu->a -= n;
			cpu->f_z(cpu->a == 0);
		}

		void sbc_a_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n;

			switch (inst->opcode)
			{
			case 0x9f: n = cpu->a; break;
			case 0x98: n = cpu->b; break;
			case 0x99: n = cpu->c; break;
			case 0x9a: n = cpu->d; break;
			case 0x9b: n = cpu->e; break;
			case 0x9c: n = cpu->h; break;
			case 0x9d: n = cpu->l; break;
			case 0x9e: n = mem->read(cpu->hl()); break;
			case 0xde: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			const uint8 c = (uint8)cpu->f_c();

			cpu->f_n(true);
			cpu->f_h((cpu->a & 0xf) - ((n & 0xf) + c) < 0);
			cpu->f_c(cpu->a < (n + c));
			cpu->a -= n + c;
			cpu->f_z(cpu->a == 0);
		}

		void and_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xa7: n = cpu->a; break;
			case 0xa0: n = cpu->b; break;
			case 0xa1: n = cpu->c; break;
			case 0xa2: n = cpu->d; break;
			case 0xa3: n = cpu->e; break;
			case 0xa4: n = cpu->h; break;
			case 0xa5: n = cpu->l; break;
			case 0xa6: n = mem->read(cpu->hl()); break;
			case 0xe6: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->a &= n;

			cpu->f_z(cpu->a == 0);
			cpu->f_n(false);
			cpu->f_h(true);
			cpu->f_c(false);
		}

		void or_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xb7: n = cpu->a; break;
			case 0xb0: n = cpu->b; break;
			case 0xb1: n = cpu->c; break;
			case 0xb2: n = cpu->d; break;
			case 0xb3: n = cpu->e; break;
			case 0xb4: n = cpu->h; break;
			case 0xb5: n = cpu->l; break;
			case 0xb6: n = mem->read(cpu->hl()); break;
			case 0xf6: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->a |= n;

			cpu->f_z(cpu->a == 0);
			cpu->f_n(false);
			cpu->f_h(false);
			cpu->f_c(false);
		}

		void xor_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xaf: n = cpu->a; break;
			case 0xa8: n = cpu->b; break;
			case 0xa9: n = cpu->c; break;
			case 0xaa: n = cpu->d; break;
			case 0xab: n = cpu->e; break;
			case 0xac: n = cpu->h; break;
			case 0xad: n = cpu->l; break;
			case 0xae: n = mem->read(cpu->hl()); break;
			case 0xee: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->a ^= n;

			cpu->f_z(cpu->a == 0);
			cpu->f_n(false);
			cpu->f_h(false);
			cpu->f_c(false);
		}

		void cp_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			uint8 n = 0;

			switch (inst->opcode)
			{
			case 0xbf: n = cpu->a; break;
			case 0xb8: n = cpu->b; break;
			case 0xb9: n = cpu->c; break;
			case 0xba: n = cpu->d; break;
			case 0xbb: n = cpu->e; break;
			case 0xbc: n = cpu->h; break;
			case 0xbd: n = cpu->l; break;
			case 0xbe: n = mem->read(cpu->hl()); break;
			case 0xfe: n = mem->read(cpu->pc + 1); break;
			default: return;
			}

			cpu->f_z(cpu->a == n);
			cpu->f_n(true);
			cpu->f_h((cpu->a & 0xf) - (n & 0xf) < 0);
			cpu->f_c(cpu->a < n);
		}

		void inc_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3c: p = &cpu->a; break;
			case 0x04: p = &cpu->b; break;
			case 0x0c: p = &cpu->c; break;
			case 0x14: p = &cpu->d; break;
			case 0x1c: p = &cpu->e; break;
			case 0x24: p = &cpu->h; break;
			case 0x2c: p = &cpu->l; break;
			default: return;
			}

			cpu->f_n(false);
			cpu->f_h((*p & 0xf) == 0xf);

			const uint8 result = *p + 1;

			*p = result;
			cpu->f_z(result == 0);
		}

		void inc_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x34
			uint8 value = mem->read(cpu->hl());

			cpu->f_n(false);
			cpu->f_h((value & 0xf) == 0xf);

			const uint8 result = value + 1;

			mem->write(cpu->hl(), result);
			cpu->f_z(result == 0);
		}

		void dec_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3d: p = &cpu->a; break;
			case 0x05: p = &cpu->b; break;
			case 0x0d: p = &cpu->c; break;
			case 0x15: p = &cpu->d; break;
			case 0x1d: p = &cpu->e; break;
			case 0x25: p = &cpu->h; break;
			case 0x2d: p = &cpu->l; break;
			default: return;
			}

			cpu->f_n(true);
			cpu->f_h((*p & 0xf) == 0);

			const uint8 result = *p - 1;

			*p = result;

			cpu->f_z(result == 0);
		}

		void dec_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x35
			uint8 value = mem->read(cpu->hl());

			cpu->f_n(true);
			cpu->f_h((value & 0xf) == 0);

			const uint8 result = value - 1;

			mem->write(cpu->hl(), result);
			cpu->f_z(result == 0);
		}

		void add_hl_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint16 n;

			switch (inst->opcode)
			{
			case 0x09: n = cpu->bc(); break;
			case 0x19: n = cpu->de(); break;
			case 0x29: n = cpu->hl(); break;
			case 0x39: n = cpu->sp; break;
			default: return;
			}

			cpu->f_n(false);
			cpu->f_h((cpu->hl() & 0x7ff) + (n & 0x7ff) > 0x7ff);
			cpu->f_c((cpu->hl() & 0xffff) + (n & 0xffff) > 0xffff);
			cpu->hl(cpu->hl() + n);
		}

		void add_sp_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0xe8
			const int8 n = mem->read(cpu->pc + 1);
			cpu->f = 0;
			cpu->f_h((cpu->sp & 0xf) + (n & 0xf) > 0xf);
			cpu->f_c((cpu->sp & 0xff) + (n & 0xff) > 0xff);
			cpu->sp += n;
		}

		void inc16_(CPU* cpu, Memory*, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x03: cpu->bc(cpu->bc() + 1); return;
			case 0x13: cpu->de(cpu->de() + 1); return;

			case 0x22: //LDI (HL),A
			case 0x2a: //LDI A,(HL)
			case 0x23: cpu->hl(cpu->hl() + 1); return;

			case 0x33: ++cpu->sp; return;
			}
		}

		void dec16_(CPU* cpu, Memory*, const Instruction* inst)
		{
			switch (inst->opcode)
			{
			case 0x0b: cpu->bc(cpu->bc() - 1); return;
			case 0x1b: cpu->de(cpu->de() - 1); return;

			case 0x32: // LDD (HL),A
			case 0x3a: // LDD A,(HL)
			case 0x2b: cpu->hl(cpu->hl() - 1); return;

			case 0x3b: --cpu->sp; return;
			default: return;
			}
		}

		// Misc
		// (SWAP, DAA, CPL, CCF, SCF, NOP, HALT, STOP, DI, EI)

		void swap_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x37: p = &cpu->a; break;
			case 0x30: p = &cpu->b; break;
			case 0x31: p = &cpu->c; break;
			case 0x32: p = &cpu->d; break;
			case 0x33: p = &cpu->e; break;
			case 0x34: p = &cpu->h; break;
			case 0x35: p = &cpu->l; break;
			default: return;
			}

			const uint8 result = (*p >> 4) | (*p << 4);

			*p = result;

			cpu->f = 0;
			cpu->f_z(result == 0);
		}

		void swap_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x36
			uint8 value = mem->read(cpu->hl());

			const uint8 result = (value >> 4) | (value << 4);

			mem->write(cpu->hl(), result);

			cpu->f = 0;
			cpu->f_z(result == 0);
		}

		void daa_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x27
			int16 result = cpu->a;

			if (cpu->f_n())
			{
				if (cpu->f_h())
				{
					result = (result - 0x6) & 0xff;
				}

				if (cpu->f_c())
				{
					result -= 0x60;
				}
			}
			else
			{
				if (cpu->f_h() || (result & 0xf) > 0x9)
				{
					result += 0x6;
				}

				if (cpu->f_c() || result > 0x9f)
				{
					result += 0x60;
				}
			}

			cpu->f_c(cpu->f_c() || ((result & 0x100) == 0x100));

			cpu->a = result & 0xff;

			cpu->f_z(cpu->a == 0);
			cpu->f_h(false);
		}

		void cpl_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x2f
			cpu->a ^= 0xff;
			cpu->f_n(true);
			cpu->f_h(true);
		}

		void ccf_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x3f
			cpu->f_c(!cpu->f_c());
			cpu->f_n(false);
			cpu->f_h(false);
		}

		void scf_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x37
			cpu->f_c(true);
			cpu->f_n(false);
			cpu->f_h(false);
		}

		void nop_(CPU*, Memory*, const Instruction*)
		{
			// opcode: 0x00
		}

		void halt_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x76

			// HALT
			// 割り込みの状況によって低電力モードになったりならなかったりする

			if (cpu->ime)
			{
				cpu->powerSavingMode_ = true;
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
					cpu->powerSavingMode_ = true;
				}
			}
		}

		void stop_(CPU*, Memory*, const Instruction*)
		{
			// opcode: 0x10
			//...
		}

		void di_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0xf3
			cpu->ime = false;
		}

		void ei_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0xfb
			cpu->imeScheduled = true;
		}

		// Rotates & Shifts
		// (RLCA, RLA, RRCA, RRA, RLC, RL, RRA, RR)

		void rlca_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x07
			const uint8 bit7 = cpu->a >> 7;
			cpu->f = 0;
			cpu->f_c(bit7 == 1);
			cpu->a = (cpu->a << 1) | bit7;
		}

		void rla_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x17
			const uint8 bit7 = cpu->a >> 7;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit7 == 1);
			cpu->a = (cpu->a << 1) | c;
		}

		void rrca_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x0f
			const uint8 bit0 = cpu->a & 1;
			cpu->f = 0;
			cpu->f_c(bit0 == 1);
			cpu->a = (cpu->a >> 1) | (bit0 << 7);
		}

		void rra_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0x1f
			const uint8 bit0 = cpu->a & 1;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit0 == 1);
			cpu->a = (cpu->a >> 1) | (c << 7);
		}

		void rlc_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x07: p = &cpu->a; break;
			case 0x00: p = &cpu->b; break;
			case 0x01: p = &cpu->c; break;
			case 0x02: p = &cpu->d; break;
			case 0x03: p = &cpu->e; break;
			case 0x04: p = &cpu->h; break;
			case 0x05: p = &cpu->l; break;
			default: return;
			}

			const uint8 bit7 = *p >> 7;
			cpu->f = 0;
			cpu->f_z(*p == 0);
			cpu->f_c(bit7 == 1);

			const uint8 result = (*p << 1) | bit7;

			*p = result;
		}

		void rlc_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x06
			uint8 value = mem->read(cpu->hl());

			const uint8 bit7 = value >> 7;
			cpu->f = 0;
			cpu->f_z(value == 0);
			cpu->f_c(bit7 == 1);

			const uint8 result = (value << 1) | bit7;

			mem->write(cpu->hl(), result);
		}

		void rl_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x17: p = &cpu->a; break;
			case 0x10: p = &cpu->b; break;
			case 0x11: p = &cpu->c; break;
			case 0x12: p = &cpu->d; break;
			case 0x13: p = &cpu->e; break;
			case 0x14: p = &cpu->h; break;
			case 0x15: p = &cpu->l; break;
			default: return;
			}

			const uint8 bit7 = *p >> 7;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit7 == 1);

			const uint8 result = (*p << 1) | c;

			*p = result;

			cpu->f_z(result == 0);
		}

		void rl_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x16
			uint8 value = mem->read(cpu->hl());

			const uint8 bit7 = value >> 7;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit7 == 1);

			const uint8 result = (value << 1) | c;

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		void rrc_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x0f: p = &cpu->a; break;
			case 0x08: p = &cpu->b; break;
			case 0x09: p = &cpu->c; break;
			case 0x0a: p = &cpu->d; break;
			case 0x0b: p = &cpu->e; break;
			case 0x0c: p = &cpu->h; break;
			case 0x0d: p = &cpu->l; break;
			default: return;
			}

			const uint8 bit0 = *p & 1;
			cpu->f = 0;
			cpu->f_c(bit0 == 1);

			const uint8 result = (*p >> 1) | (bit0 << 7);

			*p = result;

			cpu->f_z(*p == 0);
		}

		void rrc_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x0e
			uint8 value = mem->read(cpu->hl());

			const uint8 bit0 = value & 1;
			cpu->f = 0;
			cpu->f_c(bit0 == 1);

			const uint8 result = (value >> 1) | (bit0 << 7);

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		void rr_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x1f: p = &cpu->a; break;
			case 0x18: p = &cpu->b; break;
			case 0x19: p = &cpu->c; break;
			case 0x1a: p = &cpu->d; break;
			case 0x1b: p = &cpu->e; break;
			case 0x1c: p = &cpu->h; break;
			case 0x1d: p = &cpu->l; break;
			default: return;
			}

			const uint8 bit0 = *p & 1;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit0 == 1);

			const uint8 result = (*p >> 1) | (c << 7);

			*p = result;

			cpu->f_z(result == 0);
		}

		void rr_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x1e
			uint8 value = mem->read(cpu->hl());

			const uint8 bit0 = value & 1;
			const uint8 c = (uint8)cpu->f_c();
			cpu->f = 0;
			cpu->f_c(bit0 == 1);

			const uint8 result = (value >> 1) | (c << 7);

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		void sla_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x27: p = &cpu->a; break;
			case 0x20: p = &cpu->b; break;
			case 0x21: p = &cpu->c; break;
			case 0x22: p = &cpu->d; break;
			case 0x23: p = &cpu->e; break;
			case 0x24: p = &cpu->h; break;
			case 0x25: p = &cpu->l; break;
			default: return;
			}

			cpu->f = 0;
			cpu->f_c(*p & (1 << 7));

			const uint8 result = *p << 1;

			*p = result;

			cpu->f_z(result == 0);
		}

		void sla_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x26
			uint8 value = mem->read(cpu->hl());

			cpu->f = 0;
			cpu->f_c(value & (1 << 7));

			const uint8 result = value << 1;

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		void sra_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x2f: p = &cpu->a; break;
			case 0x28: p = &cpu->b; break;
			case 0x29: p = &cpu->c; break;
			case 0x2a: p = &cpu->d; break;
			case 0x2b: p = &cpu->e; break;
			case 0x2c: p = &cpu->h; break;
			case 0x2d: p = &cpu->l; break;
			default: return;
			}

			cpu->f = 0;
			cpu->f_c(*p & 1);

			const uint8 result = (*p >> 1) | (*p & (1 << 7));

			*p = result;

			cpu->f_z(result == 0);
		}

		void sra_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x2e
			uint8 value = mem->read(cpu->hl());

			cpu->f = 0;
			cpu->f_c(value & 1);

			const uint8 result = (value >> 1) | (value & (1 << 7));

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		void srl_(CPU* cpu, Memory*, const Instruction* inst)
		{
			uint8* p;

			switch (inst->opcode)
			{
			case 0x3f: p = &cpu->a; break;
			case 0x38: p = &cpu->b; break;
			case 0x39: p = &cpu->c; break;
			case 0x3a: p = &cpu->d; break;
			case 0x3b: p = &cpu->e; break;
			case 0x3c: p = &cpu->h; break;
			case 0x3d: p = &cpu->l; break;
			default: return;
			}

			cpu->f = 0;
			cpu->f_c(*p & 1);

			const uint8 result = *p >> 1;

			*p = result;

			cpu->f_z(result == 0);
		}

		void srl_hl_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0x3e
			uint8 value = mem->read(cpu->hl());

			cpu->f = 0;
			cpu->f_c(value & 1);

			const uint8 result = value >> 1;

			mem->write(cpu->hl(), result);

			cpu->f_z(result == 0);
		}

		// Bit
		// (BIT, SET)

		void bit_(CPU* cpu, Memory*, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0x40) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &cpu->a; break;
			case 0x0: p = &cpu->b; break;
			case 0x1: p = &cpu->c; break;
			case 0x2: p = &cpu->d; break;
			case 0x3: p = &cpu->e; break;
			case 0x4: p = &cpu->h; break;
			case 0x5: p = &cpu->l; break;
			default: return;
			}

			cpu->f_z((*p & (1 << b)) == 0);
			cpu->f_n(false);
			cpu->f_h(true);
		}

		void bit_hl_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0x40) / 8;

			uint8 value = mem->read(cpu->hl());

			cpu->f_z((value & (1 << b)) == 0);
			cpu->f_n(false);
			cpu->f_h(true);
		}

		void set_(CPU* cpu, Memory*, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0xc0) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &cpu->a; break;
			case 0x0: p = &cpu->b; break;
			case 0x1: p = &cpu->c; break;
			case 0x2: p = &cpu->d; break;
			case 0x3: p = &cpu->e; break;
			case 0x4: p = &cpu->h; break;
			case 0x5: p = &cpu->l; break;
			default: return;
			}

			const uint8 result = *p | (1 << b);

			*p = result;
		}

		void set_hl_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0xc0) / 8;

			uint8 value = mem->read(cpu->hl());

			const uint8 result = value | (1 << b);

			mem->write(cpu->hl(), result);
		}

		void res_(CPU* cpu, Memory*, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0x80) / 8;

			uint8* p;

			uint8 opl = inst->opcode & 0xf;
			if (opl > 7) opl -= 8;

			switch (opl)
			{
			case 0x7: p = &cpu->a; break;
			case 0x0: p = &cpu->b; break;
			case 0x1: p = &cpu->c; break;
			case 0x2: p = &cpu->d; break;
			case 0x3: p = &cpu->e; break;
			case 0x4: p = &cpu->h; break;
			case 0x5: p = &cpu->l; break;
			default: return;
			}

			const uint8 result = *p & ~(1 << b);

			*p = result;
		}

		void res_hl_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint8 b = (inst->opcode - 0x80) / 8;

			uint8 value = mem->read(cpu->hl());

			const uint8 result = value & ~(1 << b);

			mem->write(cpu->hl(), result);
		}

		// Jumps
		// (JP, JR)

		void jp_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const uint16 addrNext = mem->read16(cpu->pc + 1);
			bool toJump = false;

			switch (inst->opcode)
			{
			case 0xc3:
				toJump = true;
				break;
			case 0xc2:
				if (not cpu->f_z()) toJump = true;
				break;
			case 0xca:
				if (cpu->f_z()) toJump = true;
				break;
			case 0xd2:
				if (not cpu->f_c()) toJump = true;
				break;
			case 0xda:
				if (cpu->f_c()) toJump = true;
				break;
			}

			if (toJump)
			{
				cpu->addrNext = addrNext;
				return;
			}

			cpu->consumedCycles = inst->cyclesOnSkip;
		}

		void jp_hl_(CPU* cpu, Memory*, const Instruction*)
		{
			// opcode: 0xe9
			cpu->addrNext = cpu->hl();
		}

		void jr_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			const int8 n = mem->read(cpu->pc + 1);
			bool toJump = false;

			switch (inst->opcode)
			{
			case 0x18:
				toJump = true;
				break;
			case 0x20:
				if (not cpu->f_z()) toJump = true;
				break;
			case 0x28:
				if (cpu->f_z()) toJump = true;
				break;
			case 0x30:
				if (not cpu->f_c()) toJump = true;
				break;
			case 0x38:
				if (cpu->f_c()) toJump = true;
				break;
			}

			if (toJump)
			{
				cpu->addrNext = cpu->addrNext + n;
				return;
			}

			cpu->consumedCycles = inst->cyclesOnSkip;
		}

		// Calls
		// (CALL)

		void call_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			bool toCall = false;

			switch (inst->opcode)
			{
			case 0xcd:
				toCall = true;
				break;
			case 0xc4:
				if (not cpu->f_z()) toCall = true;
				break;
			case 0xcc:
				if (cpu->f_z()) toCall = true;
				break;
			case 0xd4:
				if (not cpu->f_c()) toCall = true;
				break;
			case 0xdc:
				if (cpu->f_c()) toCall = true;
				break;
			}

			if (toCall)
			{
				const uint16 addr = mem->read16(cpu->pc + 1);
				mem->write(--cpu->sp, (cpu->pc + 3) >> 8);
				mem->write(--cpu->sp, (cpu->pc + 3) & 0xff);
				cpu->addrNext = addr;
				return;
			}

			cpu->consumedCycles = inst->cyclesOnSkip;
		}

		// Restarts
		// (RST)

		void rst_(CPU* cpu, Memory* mem, const Instruction* inst)
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

			mem->write(--cpu->sp, (cpu->pc + 1) >> 8);
			mem->write(--cpu->sp, (cpu->pc + 1) & 0xff);
			cpu->addrNext = addr;
		}


		// Returns
		// (RET, RETI)

		void ret_(CPU* cpu, Memory* mem, const Instruction* inst)
		{
			bool toRet = false;

			switch (inst->opcode)
			{
			case 0xc9:
				toRet = true;
				break;
			case 0xc0:
				if (not cpu->f_z()) toRet = true;
				break;
			case 0xc8:
				if (cpu->f_z()) toRet = true;
				break;
			case 0xd0:
				if (not cpu->f_c()) toRet = true;
				break;
			case 0xd8:
				if (cpu->f_c()) toRet = true;
				break;
			}

			if (toRet)
			{
				cpu->addrNext = mem->read16(cpu->sp);
				cpu->sp += 2;
				return;
			}

			cpu->consumedCycles = inst->cyclesOnSkip;
		}

		void reti_(CPU* cpu, Memory* mem, const Instruction*)
		{
			// opcode: 0xd9
			cpu->addrNext = mem->read16(cpu->sp);
			cpu->sp += 2;
			cpu->ime = true;
		}

		// Other

		void cbprefix_(CPU*, Memory*, const Instruction*)
		{
		}

		void illegal_(CPU*, Memory*, const Instruction*)
		{
		}


		// Instructions List 0x00-0xff

		std::array<Instruction, 256> unprefixed = {
			{
				{ 0x00, 1, 4, 0, U"NOP"_sv, U""_sv, nop_ },
				{ 0x01, 3, 12, 0, U"LD"_sv, U"BC,d16"_sv, ld16_ },
				{ 0x02, 1, 8, 0, U"LD"_sv, U"BC,A"_sv, ld_n_a_ },
				{ 0x03, 1, 8, 0, U"INC"_sv, U"BC"_sv, inc16_ },
				{ 0x04, 1, 4, 0, U"INC"_sv, U"B"_sv, inc_ },
				{ 0x05, 1, 4, 0, U"DEC"_sv, U"B"_sv, dec_ },
				{ 0x06, 2, 8, 0, U"LD"_sv, U"B,d8"_sv, ld_r_n_ },
				{ 0x07, 1, 4, 0, U"RLCA"_sv, U""_sv, rlca_ },
				{ 0x08, 3, 20, 0, U"LD"_sv, U"a16,SP"_sv, ld16_n_sp_ },
				{ 0x09, 1, 8, 0, U"ADD"_sv, U"HL,BC"_sv, add_hl_ },
				{ 0x0a, 1, 8, 0, U"LD"_sv, U"A,BC"_sv, ld_a_n_ },
				{ 0x0b, 1, 8, 0, U"DEC"_sv, U"BC"_sv, dec16_ },
				{ 0x0c, 1, 4, 0, U"INC"_sv, U"C"_sv, inc_ },
				{ 0x0d, 1, 4, 0, U"DEC"_sv, U"C"_sv, dec_ },
				{ 0x0e, 2, 8, 0, U"LD"_sv, U"C,d8"_sv, ld_r_n_ },
				{ 0x0f, 1, 4, 0, U"RRCA"_sv, U""_sv, rrca_ },
				{ 0x10, 2, 4, 0, U"STOP"_sv, U"d8"_sv, stop_ },
				{ 0x11, 3, 12, 0, U"LD"_sv, U"DE,d16"_sv, ld16_ },
				{ 0x12, 1, 8, 0, U"LD"_sv, U"DE,A"_sv, ld_n_a_ },
				{ 0x13, 1, 8, 0, U"INC"_sv, U"DE"_sv, inc16_ },
				{ 0x14, 1, 4, 0, U"INC"_sv, U"D"_sv, inc_ },
				{ 0x15, 1, 4, 0, U"DEC"_sv, U"D"_sv, dec_ },
				{ 0x16, 2, 8, 0, U"LD"_sv, U"D,d8"_sv, ld_r_n_ },
				{ 0x17, 1, 4, 0, U"RLA"_sv, U""_sv, rla_ },
				{ 0x18, 2, 12, 0, U"JR"_sv, U"r8"_sv, jr_ },
				{ 0x19, 1, 8, 0, U"ADD"_sv, U"HL,DE"_sv, add_hl_ },
				{ 0x1a, 1, 8, 0, U"LD"_sv, U"A,DE"_sv, ld_a_n_ },
				{ 0x1b, 1, 8, 0, U"DEC"_sv, U"DE"_sv, dec16_ },
				{ 0x1c, 1, 4, 0, U"INC"_sv, U"E"_sv, inc_ },
				{ 0x1d, 1, 4, 0, U"DEC"_sv, U"E"_sv, dec_ },
				{ 0x1e, 2, 8, 0, U"LD"_sv, U"E,d8"_sv, ld_r_n_ },
				{ 0x1f, 1, 4, 0, U"RRA"_sv, U""_sv, rra_ },
				{ 0x20, 2, 12, 8, U"JR"_sv, U"NZ,r8"_sv, jr_ },
				{ 0x21, 3, 12, 0, U"LD"_sv, U"HL,d16"_sv, ld16_ },
				{ 0x22, 1, 8, 0, U"LDI"_sv, U"HL,A"_sv, ldi_hl_a_ },
				{ 0x23, 1, 8, 0, U"INC"_sv, U"HL"_sv, inc16_ },
				{ 0x24, 1, 4, 0, U"INC"_sv, U"H"_sv, inc_ },
				{ 0x25, 1, 4, 0, U"DEC"_sv, U"H"_sv, dec_ },
				{ 0x26, 2, 8, 0, U"LD"_sv, U"H,d8"_sv, ld_r_n_ },
				{ 0x27, 1, 4, 0, U"DAA"_sv, U""_sv, daa_ },
				{ 0x28, 2, 12, 8, U"JR"_sv, U"Z,r8"_sv, jr_ },
				{ 0x29, 1, 8, 0, U"ADD"_sv, U"HL,HL"_sv, add_hl_ },
				{ 0x2a, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, ldi_a_hl_ },
				{ 0x2b, 1, 8, 0, U"DEC"_sv, U"HL"_sv, dec16_ },
				{ 0x2c, 1, 4, 0, U"INC"_sv, U"L"_sv, inc_ },
				{ 0x2d, 1, 4, 0, U"DEC"_sv, U"L"_sv, dec_ },
				{ 0x2e, 2, 8, 0, U"LD"_sv, U"L,d8"_sv, ld_r_n_ },
				{ 0x2f, 1, 4, 0, U"CPL"_sv, U""_sv, cpl_ },
				{ 0x30, 2, 12, 8, U"JR"_sv, U"NC,r8"_sv, jr_ },
				{ 0x31, 3, 12, 0, U"LD"_sv, U"SP,d16"_sv, ld16_ },
				{ 0x32, 1, 8, 0, U"LD"_sv, U"HL,A"_sv, ldd_hl_a_ },
				{ 0x33, 1, 8, 0, U"INC"_sv, U"SP"_sv, inc16_ },
				{ 0x34, 1, 12, 0, U"INC"_sv, U"HL"_sv, inc_hl_ },
				{ 0x35, 1, 12, 0, U"DEC"_sv, U"HL"_sv, dec_hl_ },
				{ 0x36, 2, 12, 0, U"LD"_sv, U"HL,d8"_sv, ld_r_r_ },
				{ 0x37, 1, 4, 0, U"SCF"_sv, U""_sv, scf_ },
				{ 0x38, 2, 12, 8, U"JR"_sv, U"C,r8"_sv, jr_ },
				{ 0x39, 1, 8, 0, U"ADD"_sv, U"HL,SP"_sv, add_hl_ },
				{ 0x3a, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, ldd_a_hl_ },
				{ 0x3b, 1, 8, 0, U"DEC"_sv, U"SP"_sv, dec16_ },
				{ 0x3c, 1, 4, 0, U"INC"_sv, U"A"_sv, inc_ },
				{ 0x3d, 1, 4, 0, U"DEC"_sv, U"A"_sv, dec_ },
				{ 0x3e, 2, 8, 0, U"LD"_sv, U"A,d8"_sv, ld_a_n_ },
				{ 0x3f, 1, 4, 0, U"CCF"_sv, U""_sv, ccf_ },
				{ 0x40, 1, 4, 0, U"LD"_sv, U"B,B"_sv, ld_r_r_ },
				{ 0x41, 1, 4, 0, U"LD"_sv, U"B,C"_sv, ld_r_r_ },
				{ 0x42, 1, 4, 0, U"LD"_sv, U"B,D"_sv, ld_r_r_ },
				{ 0x43, 1, 4, 0, U"LD"_sv, U"B,E"_sv, ld_r_r_ },
				{ 0x44, 1, 4, 0, U"LD"_sv, U"B,H"_sv, ld_r_r_ },
				{ 0x45, 1, 4, 0, U"LD"_sv, U"B,L"_sv, ld_r_r_ },
				{ 0x46, 1, 8, 0, U"LD"_sv, U"B,HL"_sv, ld_r_r_ },
				{ 0x47, 1, 4, 0, U"LD"_sv, U"B,A"_sv, ld_n_a_ },
				{ 0x48, 1, 4, 0, U"LD"_sv, U"C,B"_sv, ld_r_r_ },
				{ 0x49, 1, 4, 0, U"LD"_sv, U"C,C"_sv, ld_r_r_ },
				{ 0x4a, 1, 4, 0, U"LD"_sv, U"C,D"_sv, ld_r_r_ },
				{ 0x4b, 1, 4, 0, U"LD"_sv, U"C,E"_sv, ld_r_r_ },
				{ 0x4c, 1, 4, 0, U"LD"_sv, U"C,H"_sv, ld_r_r_ },
				{ 0x4d, 1, 4, 0, U"LD"_sv, U"C,L"_sv, ld_r_r_ },
				{ 0x4e, 1, 8, 0, U"LD"_sv, U"C,HL"_sv, ld_r_r_ },
				{ 0x4f, 1, 4, 0, U"LD"_sv, U"C,A"_sv, ld_n_a_ },
				{ 0x50, 1, 4, 0, U"LD"_sv, U"D,B"_sv, ld_r_r_ },
				{ 0x51, 1, 4, 0, U"LD"_sv, U"D,C"_sv, ld_r_r_ },
				{ 0x52, 1, 4, 0, U"LD"_sv, U"D,D"_sv, ld_r_r_ },
				{ 0x53, 1, 4, 0, U"LD"_sv, U"D,E"_sv, ld_r_r_ },
				{ 0x54, 1, 4, 0, U"LD"_sv, U"D,H"_sv, ld_r_r_ },
				{ 0x55, 1, 4, 0, U"LD"_sv, U"D,L"_sv, ld_r_r_ },
				{ 0x56, 1, 8, 0, U"LD"_sv, U"D,HL"_sv, ld_r_r_ },
				{ 0x57, 1, 4, 0, U"LD"_sv, U"D,A"_sv, ld_n_a_ },
				{ 0x58, 1, 4, 0, U"LD"_sv, U"E,B"_sv, ld_r_r_ },
				{ 0x59, 1, 4, 0, U"LD"_sv, U"E,C"_sv, ld_r_r_ },
				{ 0x5a, 1, 4, 0, U"LD"_sv, U"E,D"_sv, ld_r_r_ },
				{ 0x5b, 1, 4, 0, U"LD"_sv, U"E,E"_sv, ld_r_r_ },
				{ 0x5c, 1, 4, 0, U"LD"_sv, U"E,H"_sv, ld_r_r_ },
				{ 0x5d, 1, 4, 0, U"LD"_sv, U"E,L"_sv, ld_r_r_ },
				{ 0x5e, 1, 8, 0, U"LD"_sv, U"E,HL"_sv, ld_r_r_ },
				{ 0x5f, 1, 4, 0, U"LD"_sv, U"E,A"_sv, ld_n_a_ },
				{ 0x60, 1, 4, 0, U"LD"_sv, U"H,B"_sv, ld_r_r_ },
				{ 0x61, 1, 4, 0, U"LD"_sv, U"H,C"_sv, ld_r_r_ },
				{ 0x62, 1, 4, 0, U"LD"_sv, U"H,D"_sv, ld_r_r_ },
				{ 0x63, 1, 4, 0, U"LD"_sv, U"H,E"_sv, ld_r_r_ },
				{ 0x64, 1, 4, 0, U"LD"_sv, U"H,H"_sv, ld_r_r_ },
				{ 0x65, 1, 4, 0, U"LD"_sv, U"H,L"_sv, ld_r_r_ },
				{ 0x66, 1, 8, 0, U"LD"_sv, U"H,HL"_sv, ld_r_r_ },
				{ 0x67, 1, 4, 0, U"LD"_sv, U"H,A"_sv, ld_n_a_ },
				{ 0x68, 1, 4, 0, U"LD"_sv, U"L,B"_sv, ld_r_r_ },
				{ 0x69, 1, 4, 0, U"LD"_sv, U"L,C"_sv, ld_r_r_ },
				{ 0x6a, 1, 4, 0, U"LD"_sv, U"L,D"_sv, ld_r_r_ },
				{ 0x6b, 1, 4, 0, U"LD"_sv, U"L,E"_sv, ld_r_r_ },
				{ 0x6c, 1, 4, 0, U"LD"_sv, U"L,H"_sv, ld_r_r_ },
				{ 0x6d, 1, 4, 0, U"LD"_sv, U"L,L"_sv, ld_r_r_ },
				{ 0x6e, 1, 8, 0, U"LD"_sv, U"L,HL"_sv, ld_r_r_ },
				{ 0x6f, 1, 4, 0, U"LD"_sv, U"L,A"_sv, ld_n_a_ },
				{ 0x70, 1, 8, 0, U"LD"_sv, U"HL,B"_sv, ld_r_r_ },
				{ 0x71, 1, 8, 0, U"LD"_sv, U"HL,C"_sv, ld_r_r_ },
				{ 0x72, 1, 8, 0, U"LD"_sv, U"HL,D"_sv, ld_r_r_ },
				{ 0x73, 1, 8, 0, U"LD"_sv, U"HL,E"_sv, ld_r_r_ },
				{ 0x74, 1, 8, 0, U"LD"_sv, U"HL,H"_sv, ld_r_r_ },
				{ 0x75, 1, 8, 0, U"LD"_sv, U"HL,L"_sv, ld_r_r_ },
				{ 0x76, 1, 4, 0, U"HALT"_sv, U""_sv, halt_ },
				{ 0x77, 1, 8, 0, U"LD"_sv, U"HL,A"_sv, ld_n_a_ },
				{ 0x78, 1, 4, 0, U"LD"_sv, U"A,B"_sv, ld_a_n_ },
				{ 0x79, 1, 4, 0, U"LD"_sv, U"A,C"_sv, ld_a_n_ },
				{ 0x7a, 1, 4, 0, U"LD"_sv, U"A,D"_sv, ld_a_n_ },
				{ 0x7b, 1, 4, 0, U"LD"_sv, U"A,E"_sv, ld_a_n_ },
				{ 0x7c, 1, 4, 0, U"LD"_sv, U"A,H"_sv, ld_a_n_ },
				{ 0x7d, 1, 4, 0, U"LD"_sv, U"A,L"_sv, ld_a_n_ },
				{ 0x7e, 1, 8, 0, U"LD"_sv, U"A,HL"_sv, ld_a_n_ },
				{ 0x7f, 1, 4, 0, U"LD"_sv, U"A,A"_sv, ld_a_n_ },
				{ 0x80, 1, 4, 0, U"ADD"_sv, U"A,B"_sv, add_a_ },
				{ 0x81, 1, 4, 0, U"ADD"_sv, U"A,C"_sv, add_a_ },
				{ 0x82, 1, 4, 0, U"ADD"_sv, U"A,D"_sv, add_a_ },
				{ 0x83, 1, 4, 0, U"ADD"_sv, U"A,E"_sv, add_a_ },
				{ 0x84, 1, 4, 0, U"ADD"_sv, U"A,H"_sv, add_a_ },
				{ 0x85, 1, 4, 0, U"ADD"_sv, U"A,L"_sv, add_a_ },
				{ 0x86, 1, 8, 0, U"ADD"_sv, U"A,HL"_sv, add_a_ },
				{ 0x87, 1, 4, 0, U"ADD"_sv, U"A,A"_sv, add_a_ },
				{ 0x88, 1, 4, 0, U"ADC"_sv, U"A,B"_sv, adc_a_ },
				{ 0x89, 1, 4, 0, U"ADC"_sv, U"A,C"_sv, adc_a_ },
				{ 0x8a, 1, 4, 0, U"ADC"_sv, U"A,D"_sv, adc_a_ },
				{ 0x8b, 1, 4, 0, U"ADC"_sv, U"A,E"_sv, adc_a_ },
				{ 0x8c, 1, 4, 0, U"ADC"_sv, U"A,H"_sv, adc_a_ },
				{ 0x8d, 1, 4, 0, U"ADC"_sv, U"A,L"_sv, adc_a_ },
				{ 0x8e, 1, 8, 0, U"ADC"_sv, U"A,HL"_sv, adc_a_ },
				{ 0x8f, 1, 4, 0, U"ADC"_sv, U"A,A"_sv, adc_a_ },
				{ 0x90, 1, 4, 0, U"SUB"_sv, U"B"_sv, sub_a_ },
				{ 0x91, 1, 4, 0, U"SUB"_sv, U"C"_sv, sub_a_ },
				{ 0x92, 1, 4, 0, U"SUB"_sv, U"D"_sv, sub_a_ },
				{ 0x93, 1, 4, 0, U"SUB"_sv, U"E"_sv, sub_a_ },
				{ 0x94, 1, 4, 0, U"SUB"_sv, U"H"_sv, sub_a_ },
				{ 0x95, 1, 4, 0, U"SUB"_sv, U"L"_sv, sub_a_ },
				{ 0x96, 1, 8, 0, U"SUB"_sv, U"HL"_sv, sub_a_ },
				{ 0x97, 1, 4, 0, U"SUB"_sv, U"A"_sv, sub_a_ },
				{ 0x98, 1, 4, 0, U"SBC"_sv, U"A,B"_sv, sbc_a_ },
				{ 0x99, 1, 4, 0, U"SBC"_sv, U"A,C"_sv, sbc_a_ },
				{ 0x9a, 1, 4, 0, U"SBC"_sv, U"A,D"_sv, sbc_a_ },
				{ 0x9b, 1, 4, 0, U"SBC"_sv, U"A,E"_sv, sbc_a_ },
				{ 0x9c, 1, 4, 0, U"SBC"_sv, U"A,H"_sv, sbc_a_ },
				{ 0x9d, 1, 4, 0, U"SBC"_sv, U"A,L"_sv, sbc_a_ },
				{ 0x9e, 1, 8, 0, U"SBC"_sv, U"A,HL"_sv, sbc_a_ },
				{ 0x9f, 1, 4, 0, U"SBC"_sv, U"A,A"_sv, sbc_a_ },
				{ 0xa0, 1, 4, 0, U"AND"_sv, U"B"_sv, and_ },
				{ 0xa1, 1, 4, 0, U"AND"_sv, U"C"_sv, and_ },
				{ 0xa2, 1, 4, 0, U"AND"_sv, U"D"_sv, and_ },
				{ 0xa3, 1, 4, 0, U"AND"_sv, U"E"_sv, and_ },
				{ 0xa4, 1, 4, 0, U"AND"_sv, U"H"_sv, and_ },
				{ 0xa5, 1, 4, 0, U"AND"_sv, U"L"_sv, and_ },
				{ 0xa6, 1, 8, 0, U"AND"_sv, U"HL"_sv, and_ },
				{ 0xa7, 1, 4, 0, U"AND"_sv, U"A"_sv, and_ },
				{ 0xa8, 1, 4, 0, U"XOR"_sv, U"B"_sv, xor_ },
				{ 0xa9, 1, 4, 0, U"XOR"_sv, U"C"_sv, xor_ },
				{ 0xaa, 1, 4, 0, U"XOR"_sv, U"D"_sv, xor_ },
				{ 0xab, 1, 4, 0, U"XOR"_sv, U"E"_sv, xor_ },
				{ 0xac, 1, 4, 0, U"XOR"_sv, U"H"_sv, xor_ },
				{ 0xad, 1, 4, 0, U"XOR"_sv, U"L"_sv, xor_ },
				{ 0xae, 1, 8, 0, U"XOR"_sv, U"HL"_sv, xor_ },
				{ 0xaf, 1, 4, 0, U"XOR"_sv, U"A"_sv, xor_ },
				{ 0xb0, 1, 4, 0, U"OR"_sv, U"B"_sv, or_ },
				{ 0xb1, 1, 4, 0, U"OR"_sv, U"C"_sv, or_ },
				{ 0xb2, 1, 4, 0, U"OR"_sv, U"D"_sv, or_ },
				{ 0xb3, 1, 4, 0, U"OR"_sv, U"E"_sv, or_ },
				{ 0xb4, 1, 4, 0, U"OR"_sv, U"H"_sv, or_ },
				{ 0xb5, 1, 4, 0, U"OR"_sv, U"L"_sv, or_ },
				{ 0xb6, 1, 8, 0, U"OR"_sv, U"HL"_sv, or_ },
				{ 0xb7, 1, 4, 0, U"OR"_sv, U"A"_sv, or_ },
				{ 0xb8, 1, 4, 0, U"CP"_sv, U"B"_sv, cp_ },
				{ 0xb9, 1, 4, 0, U"CP"_sv, U"C"_sv, cp_ },
				{ 0xba, 1, 4, 0, U"CP"_sv, U"D"_sv, cp_ },
				{ 0xbb, 1, 4, 0, U"CP"_sv, U"E"_sv, cp_ },
				{ 0xbc, 1, 4, 0, U"CP"_sv, U"H"_sv, cp_ },
				{ 0xbd, 1, 4, 0, U"CP"_sv, U"L"_sv, cp_ },
				{ 0xbe, 1, 8, 0, U"CP"_sv, U"HL"_sv, cp_ },
				{ 0xbf, 1, 4, 0, U"CP"_sv, U"A"_sv, cp_ },
				{ 0xc0, 1, 20, 8, U"RET"_sv, U"NZ"_sv, ret_ },
				{ 0xc1, 1, 12, 0, U"POP"_sv, U"BC"_sv, pop_ },
				{ 0xc2, 3, 16, 12, U"JP"_sv, U"NZ,a16"_sv, jp_ },
				{ 0xc3, 3, 16, 0, U"JP"_sv, U"a16"_sv, jp_ },
				{ 0xc4, 3, 24, 12, U"CALL"_sv, U"NZ,a16"_sv, call_ },
				{ 0xc5, 1, 16, 0, U"PUSH"_sv, U"BC"_sv, push_ },
				{ 0xc6, 2, 8, 0, U"ADD"_sv, U"A,d8"_sv, add_a_ },
				{ 0xc7, 1, 16, 0, U"RST"_sv, U"00H"_sv, rst_ },
				{ 0xc8, 1, 20, 8, U"RET"_sv, U"Z"_sv, ret_ },
				{ 0xc9, 1, 16, 0, U"RET"_sv, U""_sv, ret_ },
				{ 0xca, 3, 16, 12, U"JP"_sv, U"Z,a16"_sv, jp_ },
				{ 0xcb, 1, 4, 0, U"PREFIX"_sv, U""_sv, cbprefix_ },
				{ 0xcc, 3, 24, 12, U"CALL"_sv, U"Z,a16"_sv, call_ },
				{ 0xcd, 3, 24, 0, U"CALL"_sv, U"a16"_sv, call_ },
				{ 0xce, 2, 8, 0, U"ADC"_sv, U"A,d8"_sv, adc_a_ },
				{ 0xcf, 1, 16, 0, U"RST"_sv, U"08H"_sv, rst_ },
				{ 0xd0, 1, 20, 8, U"RET"_sv, U"NC"_sv, ret_ },
				{ 0xd1, 1, 12, 0, U"POP"_sv, U"DE"_sv, pop_ },
				{ 0xd2, 3, 16, 12, U"JP"_sv, U"NC,a16"_sv, jp_ },
				{ 0xd3, 1, 4, 0, U"ILLEGAL_D3"_sv, U""_sv, illegal_ },
				{ 0xd4, 3, 24, 12, U"CALL"_sv, U"NC,a16"_sv, call_ },
				{ 0xd5, 1, 16, 0, U"PUSH"_sv, U"DE"_sv, push_ },
				{ 0xd6, 2, 8, 0, U"SUB"_sv, U"d8"_sv, sub_a_ },
				{ 0xd7, 1, 16, 0, U"RST"_sv, U"10H"_sv, rst_ },
				{ 0xd8, 1, 20, 8, U"RET"_sv, U"C"_sv, ret_ },
				{ 0xd9, 1, 16, 0, U"RETI"_sv, U""_sv, reti_ },
				{ 0xda, 3, 16, 12, U"JP"_sv, U"C,a16"_sv, jp_ },
				{ 0xdb, 1, 4, 0, U"ILLEGAL_DB"_sv, U""_sv, illegal_ },
				{ 0xdc, 3, 24, 12, U"CALL"_sv, U"C,a16"_sv, call_ },
				{ 0xdd, 1, 4, 0, U"ILLEGAL_DD"_sv, U""_sv, illegal_ },
				{ 0xde, 2, 8, 0, U"SBC"_sv, U"A,d8"_sv, sbc_a_ },
				{ 0xdf, 1, 16, 0, U"RST"_sv, U"18H"_sv, rst_ },
				{ 0xe0, 2, 12, 0, U"LDH"_sv, U"a8,A"_sv, ldh_ },
				{ 0xe1, 1, 12, 0, U"POP"_sv, U"HL"_sv, pop_ },
				{ 0xe2, 1, 8, 0, U"LD"_sv, U"C,A"_sv, ld_c_a_ },
				{ 0xe3, 1, 4, 0, U"ILLEGAL_E3"_sv, U""_sv, illegal_ },
				{ 0xe4, 1, 4, 0, U"ILLEGAL_E4"_sv, U""_sv, illegal_ },
				{ 0xe5, 1, 16, 0, U"PUSH"_sv, U"HL"_sv, push_ },
				{ 0xe6, 2, 8, 0, U"AND"_sv, U"d8"_sv, and_ },
				{ 0xe7, 1, 16, 0, U"RST"_sv, U"20H"_sv, rst_ },
				{ 0xe8, 2, 16, 0, U"ADD"_sv, U"SP,r8"_sv, add_sp_ },
				{ 0xe9, 1, 4, 0, U"JP"_sv, U"HL"_sv, jp_hl_ },
				{ 0xea, 3, 16, 0, U"LD"_sv, U"a16,A"_sv, ld_n_a_ },
				{ 0xeb, 1, 4, 0, U"ILLEGAL_EB"_sv, U""_sv, illegal_ },
				{ 0xec, 1, 4, 0, U"ILLEGAL_EC"_sv, U""_sv, illegal_ },
				{ 0xed, 1, 4, 0, U"ILLEGAL_ED"_sv, U""_sv, illegal_ },
				{ 0xee, 2, 8, 0, U"XOR"_sv, U"d8"_sv, xor_ },
				{ 0xef, 1, 16, 0, U"RST"_sv, U"28H"_sv, rst_ },
				{ 0xf0, 2, 12, 0, U"LDH"_sv, U"A,a8"_sv, ldh_ },
				{ 0xf1, 1, 12, 0, U"POP"_sv, U"AF"_sv, pop_ },
				{ 0xf2, 1, 8, 0, U"LD"_sv, U"A,C"_sv, ld_a_c_ },
				{ 0xf3, 1, 4, 0, U"DI"_sv, U""_sv, di_ },
				{ 0xf4, 1, 4, 0, U"ILLEGAL_F4"_sv, U""_sv, illegal_ },
				{ 0xf5, 1, 16, 0, U"PUSH"_sv, U"AF"_sv, push_ },
				{ 0xf6, 2, 8, 0, U"OR"_sv, U"d8"_sv, or_ },
				{ 0xf7, 1, 16, 0, U"RST"_sv, U"30H"_sv, rst_ },
				{ 0xf8, 2, 12, 0, U"LD"_sv, U"HL,SP,r8"_sv, ldhl_sp_n_ },
				{ 0xf9, 1, 8, 0, U"LD"_sv, U"SP,HL"_sv, ld16_sp_hl_ },
				{ 0xfa, 3, 16, 0, U"LD"_sv, U"A,a16"_sv, ld_a_n_ },
				{ 0xfb, 1, 4, 0, U"EI"_sv, U""_sv, ei_ },
				{ 0xfc, 1, 4, 0, U"ILLEGAL_FC"_sv, U""_sv, illegal_ },
				{ 0xfd, 1, 4, 0, U"ILLEGAL_FD"_sv, U""_sv, illegal_ },
				{ 0xfe, 2, 8, 0, U"CP"_sv, U"d8"_sv, cp_ },
				{ 0xff, 1, 16, 0, U"RST"_sv, U"38H"_sv, rst_ },
			}
		};

		// Instructions List (0xcb Prefixed) 0x00-0xff

		std::array<Instruction, 256> cbprefixed = {
			{
				{ 0x00, 2, 8, 0, U"RLC"_sv, U"B"_sv, rlc_ },
				{ 0x01, 2, 8, 0, U"RLC"_sv, U"C"_sv, rlc_ },
				{ 0x02, 2, 8, 0, U"RLC"_sv, U"D"_sv, rlc_ },
				{ 0x03, 2, 8, 0, U"RLC"_sv, U"E"_sv, rlc_ },
				{ 0x04, 2, 8, 0, U"RLC"_sv, U"H"_sv, rlc_ },
				{ 0x05, 2, 8, 0, U"RLC"_sv, U"L"_sv, rlc_ },
				{ 0x06, 2, 16, 0, U"RLC"_sv, U"HL"_sv, rlc_hl_ },
				{ 0x07, 2, 8, 0, U"RLC"_sv, U"A"_sv, rlc_ },
				{ 0x08, 2, 8, 0, U"RRC"_sv, U"B"_sv, rrc_ },
				{ 0x09, 2, 8, 0, U"RRC"_sv, U"C"_sv, rrc_ },
				{ 0x0a, 2, 8, 0, U"RRC"_sv, U"D"_sv, rrc_ },
				{ 0x0b, 2, 8, 0, U"RRC"_sv, U"E"_sv, rrc_ },
				{ 0x0c, 2, 8, 0, U"RRC"_sv, U"H"_sv, rrc_ },
				{ 0x0d, 2, 8, 0, U"RRC"_sv, U"L"_sv, rrc_ },
				{ 0x0e, 2, 16, 0, U"RRC"_sv, U"HL"_sv, rrc_hl_ },
				{ 0x0f, 2, 8, 0, U"RRC"_sv, U"A"_sv, rrc_ },
				{ 0x10, 2, 8, 0, U"RL"_sv, U"B"_sv, rl_ },
				{ 0x11, 2, 8, 0, U"RL"_sv, U"C"_sv, rl_ },
				{ 0x12, 2, 8, 0, U"RL"_sv, U"D"_sv, rl_ },
				{ 0x13, 2, 8, 0, U"RL"_sv, U"E"_sv, rl_ },
				{ 0x14, 2, 8, 0, U"RL"_sv, U"H"_sv, rl_ },
				{ 0x15, 2, 8, 0, U"RL"_sv, U"L"_sv, rl_ },
				{ 0x16, 2, 16, 0, U"RL"_sv, U"HL"_sv, rl_hl_ },
				{ 0x17, 2, 8, 0, U"RL"_sv, U"A"_sv, rl_ },
				{ 0x18, 2, 8, 0, U"RR"_sv, U"B"_sv, rr_ },
				{ 0x19, 2, 8, 0, U"RR"_sv, U"C"_sv, rr_ },
				{ 0x1a, 2, 8, 0, U"RR"_sv, U"D"_sv, rr_ },
				{ 0x1b, 2, 8, 0, U"RR"_sv, U"E"_sv, rr_ },
				{ 0x1c, 2, 8, 0, U"RR"_sv, U"H"_sv, rr_ },
				{ 0x1d, 2, 8, 0, U"RR"_sv, U"L"_sv, rr_ },
				{ 0x1e, 2, 16, 0, U"RR"_sv, U"HL"_sv, rr_hl_ },
				{ 0x1f, 2, 8, 0, U"RR"_sv, U"A"_sv, rr_ },
				{ 0x20, 2, 8, 0, U"SLA"_sv, U"B"_sv, sla_ },
				{ 0x21, 2, 8, 0, U"SLA"_sv, U"C"_sv, sla_ },
				{ 0x22, 2, 8, 0, U"SLA"_sv, U"D"_sv, sla_ },
				{ 0x23, 2, 8, 0, U"SLA"_sv, U"E"_sv, sla_ },
				{ 0x24, 2, 8, 0, U"SLA"_sv, U"H"_sv, sla_ },
				{ 0x25, 2, 8, 0, U"SLA"_sv, U"L"_sv, sla_ },
				{ 0x26, 2, 16, 0, U"SLA"_sv, U"HL"_sv, sla_hl_ },
				{ 0x27, 2, 8, 0, U"SLA"_sv, U"A"_sv, sla_ },
				{ 0x28, 2, 8, 0, U"SRA"_sv, U"B"_sv, sra_ },
				{ 0x29, 2, 8, 0, U"SRA"_sv, U"C"_sv, sra_ },
				{ 0x2a, 2, 8, 0, U"SRA"_sv, U"D"_sv, sra_ },
				{ 0x2b, 2, 8, 0, U"SRA"_sv, U"E"_sv, sra_ },
				{ 0x2c, 2, 8, 0, U"SRA"_sv, U"H"_sv, sra_ },
				{ 0x2d, 2, 8, 0, U"SRA"_sv, U"L"_sv, sra_ },
				{ 0x2e, 2, 16, 0, U"SRA"_sv, U"HL"_sv, sra_hl_ },
				{ 0x2f, 2, 8, 0, U"SRA"_sv, U"A"_sv, sra_ },
				{ 0x30, 2, 8, 0, U"SWAP"_sv, U"B"_sv, swap_ },
				{ 0x31, 2, 8, 0, U"SWAP"_sv, U"C"_sv, swap_ },
				{ 0x32, 2, 8, 0, U"SWAP"_sv, U"D"_sv, swap_ },
				{ 0x33, 2, 8, 0, U"SWAP"_sv, U"E"_sv, swap_ },
				{ 0x34, 2, 8, 0, U"SWAP"_sv, U"H"_sv, swap_ },
				{ 0x35, 2, 8, 0, U"SWAP"_sv, U"L"_sv, swap_ },
				{ 0x36, 2, 16, 0, U"SWAP"_sv, U"HL"_sv, swap_hl_ },
				{ 0x37, 2, 8, 0, U"SWAP"_sv, U"A"_sv, swap_ },
				{ 0x38, 2, 8, 0, U"SRL"_sv, U"B"_sv, srl_ },
				{ 0x39, 2, 8, 0, U"SRL"_sv, U"C"_sv, srl_ },
				{ 0x3a, 2, 8, 0, U"SRL"_sv, U"D"_sv, srl_ },
				{ 0x3b, 2, 8, 0, U"SRL"_sv, U"E"_sv, srl_ },
				{ 0x3c, 2, 8, 0, U"SRL"_sv, U"H"_sv, srl_ },
				{ 0x3d, 2, 8, 0, U"SRL"_sv, U"L"_sv, srl_ },
				{ 0x3e, 2, 16, 0, U"SRL"_sv, U"HL"_sv, srl_hl_ },
				{ 0x3f, 2, 8, 0, U"SRL"_sv, U"A"_sv, srl_ },
				{ 0x40, 2, 8, 0, U"BIT"_sv, U"0,B"_sv, bit_ },
				{ 0x41, 2, 8, 0, U"BIT"_sv, U"0,C"_sv, bit_ },
				{ 0x42, 2, 8, 0, U"BIT"_sv, U"0,D"_sv, bit_ },
				{ 0x43, 2, 8, 0, U"BIT"_sv, U"0,E"_sv, bit_ },
				{ 0x44, 2, 8, 0, U"BIT"_sv, U"0,H"_sv, bit_ },
				{ 0x45, 2, 8, 0, U"BIT"_sv, U"0,L"_sv, bit_ },
				{ 0x46, 2, 12, 0, U"BIT"_sv, U"0,HL"_sv, bit_hl_ },
				{ 0x47, 2, 8, 0, U"BIT"_sv, U"0,A"_sv, bit_ },
				{ 0x48, 2, 8, 0, U"BIT"_sv, U"1,B"_sv, bit_ },
				{ 0x49, 2, 8, 0, U"BIT"_sv, U"1,C"_sv, bit_ },
				{ 0x4a, 2, 8, 0, U"BIT"_sv, U"1,D"_sv, bit_ },
				{ 0x4b, 2, 8, 0, U"BIT"_sv, U"1,E"_sv, bit_ },
				{ 0x4c, 2, 8, 0, U"BIT"_sv, U"1,H"_sv, bit_ },
				{ 0x4d, 2, 8, 0, U"BIT"_sv, U"1,L"_sv, bit_ },
				{ 0x4e, 2, 12, 0, U"BIT"_sv, U"1,HL"_sv, bit_hl_ },
				{ 0x4f, 2, 8, 0, U"BIT"_sv, U"1,A"_sv, bit_ },
				{ 0x50, 2, 8, 0, U"BIT"_sv, U"2,B"_sv, bit_ },
				{ 0x51, 2, 8, 0, U"BIT"_sv, U"2,C"_sv, bit_ },
				{ 0x52, 2, 8, 0, U"BIT"_sv, U"2,D"_sv, bit_ },
				{ 0x53, 2, 8, 0, U"BIT"_sv, U"2,E"_sv, bit_ },
				{ 0x54, 2, 8, 0, U"BIT"_sv, U"2,H"_sv, bit_ },
				{ 0x55, 2, 8, 0, U"BIT"_sv, U"2,L"_sv, bit_ },
				{ 0x56, 2, 12, 0, U"BIT"_sv, U"2,HL"_sv, bit_hl_ },
				{ 0x57, 2, 8, 0, U"BIT"_sv, U"2,A"_sv, bit_ },
				{ 0x58, 2, 8, 0, U"BIT"_sv, U"3,B"_sv, bit_ },
				{ 0x59, 2, 8, 0, U"BIT"_sv, U"3,C"_sv, bit_ },
				{ 0x5a, 2, 8, 0, U"BIT"_sv, U"3,D"_sv, bit_ },
				{ 0x5b, 2, 8, 0, U"BIT"_sv, U"3,E"_sv, bit_ },
				{ 0x5c, 2, 8, 0, U"BIT"_sv, U"3,H"_sv, bit_ },
				{ 0x5d, 2, 8, 0, U"BIT"_sv, U"3,L"_sv, bit_ },
				{ 0x5e, 2, 12, 0, U"BIT"_sv, U"3,HL"_sv, bit_hl_ },
				{ 0x5f, 2, 8, 0, U"BIT"_sv, U"3,A"_sv, bit_ },
				{ 0x60, 2, 8, 0, U"BIT"_sv, U"4,B"_sv, bit_ },
				{ 0x61, 2, 8, 0, U"BIT"_sv, U"4,C"_sv, bit_ },
				{ 0x62, 2, 8, 0, U"BIT"_sv, U"4,D"_sv, bit_ },
				{ 0x63, 2, 8, 0, U"BIT"_sv, U"4,E"_sv, bit_ },
				{ 0x64, 2, 8, 0, U"BIT"_sv, U"4,H"_sv, bit_ },
				{ 0x65, 2, 8, 0, U"BIT"_sv, U"4,L"_sv, bit_ },
				{ 0x66, 2, 12, 0, U"BIT"_sv, U"4,HL"_sv, bit_hl_ },
				{ 0x67, 2, 8, 0, U"BIT"_sv, U"4,A"_sv, bit_ },
				{ 0x68, 2, 8, 0, U"BIT"_sv, U"5,B"_sv, bit_ },
				{ 0x69, 2, 8, 0, U"BIT"_sv, U"5,C"_sv, bit_ },
				{ 0x6a, 2, 8, 0, U"BIT"_sv, U"5,D"_sv, bit_ },
				{ 0x6b, 2, 8, 0, U"BIT"_sv, U"5,E"_sv, bit_ },
				{ 0x6c, 2, 8, 0, U"BIT"_sv, U"5,H"_sv, bit_ },
				{ 0x6d, 2, 8, 0, U"BIT"_sv, U"5,L"_sv, bit_ },
				{ 0x6e, 2, 12, 0, U"BIT"_sv, U"5,HL"_sv, bit_hl_ },
				{ 0x6f, 2, 8, 0, U"BIT"_sv, U"5,A"_sv, bit_ },
				{ 0x70, 2, 8, 0, U"BIT"_sv, U"6,B"_sv, bit_ },
				{ 0x71, 2, 8, 0, U"BIT"_sv, U"6,C"_sv, bit_ },
				{ 0x72, 2, 8, 0, U"BIT"_sv, U"6,D"_sv, bit_ },
				{ 0x73, 2, 8, 0, U"BIT"_sv, U"6,E"_sv, bit_ },
				{ 0x74, 2, 8, 0, U"BIT"_sv, U"6,H"_sv, bit_ },
				{ 0x75, 2, 8, 0, U"BIT"_sv, U"6,L"_sv, bit_ },
				{ 0x76, 2, 12, 0, U"BIT"_sv, U"6,HL"_sv, bit_hl_ },
				{ 0x77, 2, 8, 0, U"BIT"_sv, U"6,A"_sv, bit_ },
				{ 0x78, 2, 8, 0, U"BIT"_sv, U"7,B"_sv, bit_ },
				{ 0x79, 2, 8, 0, U"BIT"_sv, U"7,C"_sv, bit_ },
				{ 0x7a, 2, 8, 0, U"BIT"_sv, U"7,D"_sv, bit_ },
				{ 0x7b, 2, 8, 0, U"BIT"_sv, U"7,E"_sv, bit_ },
				{ 0x7c, 2, 8, 0, U"BIT"_sv, U"7,H"_sv, bit_ },
				{ 0x7d, 2, 8, 0, U"BIT"_sv, U"7,L"_sv, bit_ },
				{ 0x7e, 2, 12, 0, U"BIT"_sv, U"7,HL"_sv, bit_hl_ },
				{ 0x7f, 2, 8, 0, U"BIT"_sv, U"7,A"_sv, bit_ },
				{ 0x80, 2, 8, 0, U"RES"_sv, U"0,B"_sv, res_ },
				{ 0x81, 2, 8, 0, U"RES"_sv, U"0,C"_sv, res_ },
				{ 0x82, 2, 8, 0, U"RES"_sv, U"0,D"_sv, res_ },
				{ 0x83, 2, 8, 0, U"RES"_sv, U"0,E"_sv, res_ },
				{ 0x84, 2, 8, 0, U"RES"_sv, U"0,H"_sv, res_ },
				{ 0x85, 2, 8, 0, U"RES"_sv, U"0,L"_sv, res_ },
				{ 0x86, 2, 16, 0, U"RES"_sv, U"0,HL"_sv, res_hl_ },
				{ 0x87, 2, 8, 0, U"RES"_sv, U"0,A"_sv, res_ },
				{ 0x88, 2, 8, 0, U"RES"_sv, U"1,B"_sv, res_ },
				{ 0x89, 2, 8, 0, U"RES"_sv, U"1,C"_sv, res_ },
				{ 0x8a, 2, 8, 0, U"RES"_sv, U"1,D"_sv, res_ },
				{ 0x8b, 2, 8, 0, U"RES"_sv, U"1,E"_sv, res_ },
				{ 0x8c, 2, 8, 0, U"RES"_sv, U"1,H"_sv, res_ },
				{ 0x8d, 2, 8, 0, U"RES"_sv, U"1,L"_sv, res_ },
				{ 0x8e, 2, 16, 0, U"RES"_sv, U"1,HL"_sv, res_hl_ },
				{ 0x8f, 2, 8, 0, U"RES"_sv, U"1,A"_sv, res_ },
				{ 0x90, 2, 8, 0, U"RES"_sv, U"2,B"_sv, res_ },
				{ 0x91, 2, 8, 0, U"RES"_sv, U"2,C"_sv, res_ },
				{ 0x92, 2, 8, 0, U"RES"_sv, U"2,D"_sv, res_ },
				{ 0x93, 2, 8, 0, U"RES"_sv, U"2,E"_sv, res_ },
				{ 0x94, 2, 8, 0, U"RES"_sv, U"2,H"_sv, res_ },
				{ 0x95, 2, 8, 0, U"RES"_sv, U"2,L"_sv, res_ },
				{ 0x96, 2, 16, 0, U"RES"_sv, U"2,HL"_sv, res_hl_ },
				{ 0x97, 2, 8, 0, U"RES"_sv, U"2,A"_sv, res_ },
				{ 0x98, 2, 8, 0, U"RES"_sv, U"3,B"_sv, res_ },
				{ 0x99, 2, 8, 0, U"RES"_sv, U"3,C"_sv, res_ },
				{ 0x9a, 2, 8, 0, U"RES"_sv, U"3,D"_sv, res_ },
				{ 0x9b, 2, 8, 0, U"RES"_sv, U"3,E"_sv, res_ },
				{ 0x9c, 2, 8, 0, U"RES"_sv, U"3,H"_sv, res_ },
				{ 0x9d, 2, 8, 0, U"RES"_sv, U"3,L"_sv, res_ },
				{ 0x9e, 2, 16, 0, U"RES"_sv, U"3,HL"_sv, res_hl_ },
				{ 0x9f, 2, 8, 0, U"RES"_sv, U"3,A"_sv, res_ },
				{ 0xa0, 2, 8, 0, U"RES"_sv, U"4,B"_sv, res_ },
				{ 0xa1, 2, 8, 0, U"RES"_sv, U"4,C"_sv, res_ },
				{ 0xa2, 2, 8, 0, U"RES"_sv, U"4,D"_sv, res_ },
				{ 0xa3, 2, 8, 0, U"RES"_sv, U"4,E"_sv, res_ },
				{ 0xa4, 2, 8, 0, U"RES"_sv, U"4,H"_sv, res_ },
				{ 0xa5, 2, 8, 0, U"RES"_sv, U"4,L"_sv, res_ },
				{ 0xa6, 2, 16, 0, U"RES"_sv, U"4,HL"_sv, res_hl_ },
				{ 0xa7, 2, 8, 0, U"RES"_sv, U"4,A"_sv, res_ },
				{ 0xa8, 2, 8, 0, U"RES"_sv, U"5,B"_sv, res_ },
				{ 0xa9, 2, 8, 0, U"RES"_sv, U"5,C"_sv, res_ },
				{ 0xaa, 2, 8, 0, U"RES"_sv, U"5,D"_sv, res_ },
				{ 0xab, 2, 8, 0, U"RES"_sv, U"5,E"_sv, res_ },
				{ 0xac, 2, 8, 0, U"RES"_sv, U"5,H"_sv, res_ },
				{ 0xad, 2, 8, 0, U"RES"_sv, U"5,L"_sv, res_ },
				{ 0xae, 2, 16, 0, U"RES"_sv, U"5,HL"_sv, res_hl_ },
				{ 0xaf, 2, 8, 0, U"RES"_sv, U"5,A"_sv, res_ },
				{ 0xb0, 2, 8, 0, U"RES"_sv, U"6,B"_sv, res_ },
				{ 0xb1, 2, 8, 0, U"RES"_sv, U"6,C"_sv, res_ },
				{ 0xb2, 2, 8, 0, U"RES"_sv, U"6,D"_sv, res_ },
				{ 0xb3, 2, 8, 0, U"RES"_sv, U"6,E"_sv, res_ },
				{ 0xb4, 2, 8, 0, U"RES"_sv, U"6,H"_sv, res_ },
				{ 0xb5, 2, 8, 0, U"RES"_sv, U"6,L"_sv, res_ },
				{ 0xb6, 2, 16, 0, U"RES"_sv, U"6,HL"_sv, res_hl_ },
				{ 0xb7, 2, 8, 0, U"RES"_sv, U"6,A"_sv, res_ },
				{ 0xb8, 2, 8, 0, U"RES"_sv, U"7,B"_sv, res_ },
				{ 0xb9, 2, 8, 0, U"RES"_sv, U"7,C"_sv, res_ },
				{ 0xba, 2, 8, 0, U"RES"_sv, U"7,D"_sv, res_ },
				{ 0xbb, 2, 8, 0, U"RES"_sv, U"7,E"_sv, res_ },
				{ 0xbc, 2, 8, 0, U"RES"_sv, U"7,H"_sv, res_ },
				{ 0xbd, 2, 8, 0, U"RES"_sv, U"7,L"_sv, res_ },
				{ 0xbe, 2, 16, 0, U"RES"_sv, U"7,HL"_sv, res_hl_ },
				{ 0xbf, 2, 8, 0, U"RES"_sv, U"7,A"_sv, res_ },
				{ 0xc0, 2, 8, 0, U"SET"_sv, U"0,B"_sv, set_ },
				{ 0xc1, 2, 8, 0, U"SET"_sv, U"0,C"_sv, set_ },
				{ 0xc2, 2, 8, 0, U"SET"_sv, U"0,D"_sv, set_ },
				{ 0xc3, 2, 8, 0, U"SET"_sv, U"0,E"_sv, set_ },
				{ 0xc4, 2, 8, 0, U"SET"_sv, U"0,H"_sv, set_ },
				{ 0xc5, 2, 8, 0, U"SET"_sv, U"0,L"_sv, set_ },
				{ 0xc6, 2, 16, 0, U"SET"_sv, U"0,HL"_sv, set_hl_ },
				{ 0xc7, 2, 8, 0, U"SET"_sv, U"0,A"_sv, set_ },
				{ 0xc8, 2, 8, 0, U"SET"_sv, U"1,B"_sv, set_ },
				{ 0xc9, 2, 8, 0, U"SET"_sv, U"1,C"_sv, set_ },
				{ 0xca, 2, 8, 0, U"SET"_sv, U"1,D"_sv, set_ },
				{ 0xcb, 2, 8, 0, U"SET"_sv, U"1,E"_sv, set_ },
				{ 0xcc, 2, 8, 0, U"SET"_sv, U"1,H"_sv, set_ },
				{ 0xcd, 2, 8, 0, U"SET"_sv, U"1,L"_sv, set_ },
				{ 0xce, 2, 16, 0, U"SET"_sv, U"1,HL"_sv, set_hl_ },
				{ 0xcf, 2, 8, 0, U"SET"_sv, U"1,A"_sv, set_ },
				{ 0xd0, 2, 8, 0, U"SET"_sv, U"2,B"_sv, set_ },
				{ 0xd1, 2, 8, 0, U"SET"_sv, U"2,C"_sv, set_ },
				{ 0xd2, 2, 8, 0, U"SET"_sv, U"2,D"_sv, set_ },
				{ 0xd3, 2, 8, 0, U"SET"_sv, U"2,E"_sv, set_ },
				{ 0xd4, 2, 8, 0, U"SET"_sv, U"2,H"_sv, set_ },
				{ 0xd5, 2, 8, 0, U"SET"_sv, U"2,L"_sv, set_ },
				{ 0xd6, 2, 16, 0, U"SET"_sv, U"2,HL"_sv, set_hl_ },
				{ 0xd7, 2, 8, 0, U"SET"_sv, U"2,A"_sv, set_ },
				{ 0xd8, 2, 8, 0, U"SET"_sv, U"3,B"_sv, set_ },
				{ 0xd9, 2, 8, 0, U"SET"_sv, U"3,C"_sv, set_ },
				{ 0xda, 2, 8, 0, U"SET"_sv, U"3,D"_sv, set_ },
				{ 0xdb, 2, 8, 0, U"SET"_sv, U"3,E"_sv, set_ },
				{ 0xdc, 2, 8, 0, U"SET"_sv, U"3,H"_sv, set_ },
				{ 0xdd, 2, 8, 0, U"SET"_sv, U"3,L"_sv, set_ },
				{ 0xde, 2, 16, 0, U"SET"_sv, U"3,HL"_sv, set_hl_ },
				{ 0xdf, 2, 8, 0, U"SET"_sv, U"3,A"_sv, set_ },
				{ 0xe0, 2, 8, 0, U"SET"_sv, U"4,B"_sv, set_ },
				{ 0xe1, 2, 8, 0, U"SET"_sv, U"4,C"_sv, set_ },
				{ 0xe2, 2, 8, 0, U"SET"_sv, U"4,D"_sv, set_ },
				{ 0xe3, 2, 8, 0, U"SET"_sv, U"4,E"_sv, set_ },
				{ 0xe4, 2, 8, 0, U"SET"_sv, U"4,H"_sv, set_ },
				{ 0xe5, 2, 8, 0, U"SET"_sv, U"4,L"_sv, set_ },
				{ 0xe6, 2, 16, 0, U"SET"_sv, U"4,HL"_sv, set_hl_ },
				{ 0xe7, 2, 8, 0, U"SET"_sv, U"4,A"_sv, set_ },
				{ 0xe8, 2, 8, 0, U"SET"_sv, U"5,B"_sv, set_ },
				{ 0xe9, 2, 8, 0, U"SET"_sv, U"5,C"_sv, set_ },
				{ 0xea, 2, 8, 0, U"SET"_sv, U"5,D"_sv, set_ },
				{ 0xeb, 2, 8, 0, U"SET"_sv, U"5,E"_sv, set_ },
				{ 0xec, 2, 8, 0, U"SET"_sv, U"5,H"_sv, set_ },
				{ 0xed, 2, 8, 0, U"SET"_sv, U"5,L"_sv, set_ },
				{ 0xee, 2, 16, 0, U"SET"_sv, U"5,HL"_sv, set_hl_ },
				{ 0xef, 2, 8, 0, U"SET"_sv, U"5,A"_sv, set_ },
				{ 0xf0, 2, 8, 0, U"SET"_sv, U"6,B"_sv, set_ },
				{ 0xf1, 2, 8, 0, U"SET"_sv, U"6,C"_sv, set_ },
				{ 0xf2, 2, 8, 0, U"SET"_sv, U"6,D"_sv, set_ },
				{ 0xf3, 2, 8, 0, U"SET"_sv, U"6,E"_sv, set_ },
				{ 0xf4, 2, 8, 0, U"SET"_sv, U"6,H"_sv, set_ },
				{ 0xf5, 2, 8, 0, U"SET"_sv, U"6,L"_sv, set_ },
				{ 0xf6, 2, 16, 0, U"SET"_sv, U"6,HL"_sv, set_hl_ },
				{ 0xf7, 2, 8, 0, U"SET"_sv, U"6,A"_sv, set_ },
				{ 0xf8, 2, 8, 0, U"SET"_sv, U"7,B"_sv, set_ },
				{ 0xf9, 2, 8, 0, U"SET"_sv, U"7,C"_sv, set_ },
				{ 0xfa, 2, 8, 0, U"SET"_sv, U"7,D"_sv, set_ },
				{ 0xfb, 2, 8, 0, U"SET"_sv, U"7,E"_sv, set_ },
				{ 0xfc, 2, 8, 0, U"SET"_sv, U"7,H"_sv, set_ },
				{ 0xfd, 2, 8, 0, U"SET"_sv, U"7,L"_sv, set_ },
				{ 0xfe, 2, 16, 0, U"SET"_sv, U"7,HL"_sv, set_hl_ },
				{ 0xff, 2, 8, 0, U"SET"_sv, U"7,A"_sv, set_ },
			}
		};
	}

	CPU::CPU(Memory* mem, PPU* ppu, LCD* lcd, dmge::Timer* timer)
		: a{}, f{}, b{}, c{}, d{}, e{}, sp{}, pc{}, mem_{ mem }, ppu_{ ppu }, lcd_{ lcd }, timer_{ timer }
	{
		reset();
	}

	void CPU::reset()
	{
		af(0x01b0); // GB/SGB:0x01b0, GBP:0xffb0, GBC:0x11b0
		bc(0x0013);
		de(0x00d8);
		hl(0x014d);
		sp = 0xfffe;
		pc = 0x0100;
	}

	void CPU::run()
	{
		// HALTによって低電力モードになっている場合はPCからのフェッチ＆実行をしない
		if (powerSavingMode_)
		{
			return;
		}

		// 現在のPCの命令をフェッチし、
		// 次のPCと消費サイクルを計算、
		// フェッチした命令を実行
		// ※実行した結果、消費サイクルが書き変わる場合は consumedCycles が変更されている（ジャンプ命令でジャンプしなかった場合など）
		// ※実行した結果、次のPCが書き変わる場合は addrNext が変更されているので PC に反映する（JPやCALLなど）

		const auto& instruction = getInstruction(pc);

		addrNext = pc + instruction.bytes;
		consumedCycles = instruction.cycles;

		if (instruction.inst != nullptr)
		{
			instruction.inst(this, mem_, &instruction);
		}

		pc = addrNext;
	}

	void CPU::interrupt()
	{
		const uint8 intEnable = mem_->read(Address::IE);
		const uint8 intFlag = mem_->read(Address::IF);

		// 低電力モードから抜ける？

		if (not ime)
		{
			if ((intEnable & intFlag) != 0)
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
			if (const auto flag = intEnable & intFlag;
				flag & (1 << i))
			{
				// 割り込みを無効に
				ime = false;
				mem_->write(Address::IF, intFlag & ~(1 << i));

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

	void CPU::applyScheduledIME()
	{
		if (imeScheduled)
		{
			ime = true;
			imeScheduled = false;
		}
	}

	void CPU::dump()
	{
		const auto& inst = getInstruction(pc);

		Console.writeln(U"pc:{:04X} {:5} {:10} af:{:04X} bc:{:04X} de:{:04X} hl:{:04X} sp:{:04X} ly:{:02X} div:{:02X} tima:{:02X} tma:{:02X} tac:{:02X} ie:{:02X} if:{:02X} rom:{:02X}"_fmt(
			pc,
			inst.mnemonic, inst.operands,
			af(), bc(), de(), hl(), sp,
			lcd_->ly(),
			mem_->read(Address::DIV), mem_->read(Address::TIMA), mem_->read(Address::TMA), mem_->read(Address::TAC),
			mem_->read(Address::IE), mem_->read(Address::IF),
			mem_->romBank()
		));
	}

	const Instruction& CPU::getInstruction(uint16 addr) const
	{
		const uint8 code = mem_->read(addr);
		if (code != 0xcb) return Instructions::unprefixed[code];

		return Instructions::cbprefixed[mem_->read(addr + 1)];
	}
}
