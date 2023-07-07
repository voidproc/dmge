#include "Memory.h"
#include "MBC.h"
#include "PPU.h"
#include "LCD.h"
#include "Audio/APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Interrupt.h"
#include "DebugPrint.h"

namespace dmge
{
	Memory::Memory()
	{
	}

	Memory::~Memory()
	{
	}

	void Memory::init(PPU* ppu, APU* apu, dmge::Timer* timer, dmge::Joypad* joypad, LCD* lcd, Interrupt* interrupt)
	{
		ppu_ = ppu;
		apu_ = apu;
		timer_ = timer;
		joypad_ = joypad;
		lcd_ = lcd;
		interrupt_ = interrupt;
	}

	void Memory::reset()
	{
		const bool cgb = isCGBMode();

		writeDirect(0xff00, 0xcf); // P1
		writeDirect(0xff01, 0x00); // SB
		writeDirect(0xff02, cgb ? 0x7f : 0x7e); // SC

		write(0xff05, 0x00); // TIMA
		write(0xff06, 0x00); // TMA
		write(0xff07, 0xf8); // TAC, 0xff80 | (0x00 & ~0xff80)
		write(0xff0f, 0xe0); // IF
		write(0xff10, 0x80); // NR10
		write(0xff11, 0xbf); // NR11
		write(0xff12, 0xf3); // NR12
		write(0xff13, 0xff); // NR13
		write(0xff14, 0xbf); // NR14
		write(0xff16, 0x3f); // NR21
		write(0xff17, 0x00); // NR22
		write(0xff18, 0xff); // NR23
		write(0xff19, 0xbf); // NR24
		write(0xff1a, 0x7f); // NR30
		write(0xff1b, 0xff); // NR31
		write(0xff1c, 0x9f); // NR32
		write(0xff1e, 0xff); // NR33
		write(0xff1f, 0xbf); // NR34
		write(0xff20, 0xff); // NR41
		write(0xff21, 0x00); // NR42
		write(0xff22, 0x00); // NR43
		write(0xff23, 0xbf); // NR44
		write(0xff24, 0x77); // NR50
		write(0xff25, 0xf3); // NR51
		write(0xff26, 0xf1); // NR52 (GB:0xf1, SGB:0xf0)
		write(0xff40, 0x91); // LCDC
		write(0xff42, 0x00); // SCY
		write(0xff43, 0x00); // SCX
		write(0xff45, 0x00); // LYC
		writeDirect(0xff46, cgb ? 0x00 : 0xff); // DMA
		write(0xff47, 0xfc); // BGP
		write(0xff48, 0xff); // OBP0
		write(0xff49, 0xff); // OBP1
		write(0xff4a, 0x00); // WY
		write(0xff4b, 0x00); // WX
		write(0xffff, 0x00); // IE

		write(0xff50, 0x01); // Boot ROM enable/disable
	}

	bool Memory::loadCartridge(FilePath cartridgePath)
	{
		mbc_ = MBC::LoadCartridge(cartridgePath);

		if (not mbc_)
		{
			return false;
		}

		mem_.resize(0x10000);

		// メモリを初期状態にリセット
		reset();

		return true;
	}

	void Memory::loadSRAM()
	{
		mbc_->loadSRAM();
	}

	void Memory::saveSRAM()
	{
		mbc_->saveSRAM();
	}

	void Memory::write(uint16 addr, uint8 value)
	{
		// フック

		if (not writeHooks_.empty())
		{
			for (const auto& hook : writeHooks_)
			{
				hook(addr, value);
			}
		}

		// MBC
		// 0x0000 - 0x7fff

		if (addr <= Address::SwitchableROMBank_End)
		{
			mbc_->write(addr, value);
			return;
		}

		// VRAM
		// 0x8000 - 0x9fff

		else if (addr <= Address::VRAM_End)
		{
			vram_[vramBank_][addr - Address::VRAM] = value;
			return;
		}

		// SRAM
		// 0xa000 - 0xbfff

		else if (addr <= Address::SRAM_End)
		{
			mbc_->write(addr, value);
			return;
		}

		// WRAM
		// 0xc000 - 0xdfff

		else if (addr <= Address::WRAM0_End)
		{
			wram_[0][addr - Address::WRAM0] = value;
			return;
		}
		else if (addr <= Address::WRAM1_End)
		{
			wram_[wramBank_][addr - Address::WRAM1] = value;
			return;
		}

		// Echo of WRAM
		// 0xe000 - 0xfdff

		else if (addr <= Address::EchoRAM_End)
		{
			write(addr - 0x2000, value);
			return;
		}

		// OAM Table
		// 0xfe00 - 0xfe9f
		// ...

		// Not usable
		// 0xfea0 - 0xfeff
		// ...

		// I/O Registers
		// 0xff00 - 0xffff

		// Joypad
		// 0xff00

		else if (addr == Address::JOYP)
		{
			joypad_->writeRegister(value);
			return;
		}

		// Serial
		// 0xff01 - 0xff02
		// ...

		// Timer
		// 0xff04 - 0xff07

		else if (addr >= Address::DIV && addr <= Address::TAC)
		{
			timer_->writeRegister(addr, value);
			return;
		}

		// Interrupt Flag (IF), Interrupt enable (IE)
		// 0xff0f, 0xffff

		else if (addr == Address::IF || addr == Address::IE)
		{
			interrupt_->writeRegister(addr, value);
		}

		// APU
		// 0xff10 - 0xff26,
		// 0xff30 - 0xff3f

		else if (addr >= Address::NR10 && addr <= Address::NR51)
		{
			// Ignore if APU is off
			if ((apu_->readRegister(Address::NR52) & 0x80) == 0)
			{
				if (not isCGBMode())
				{
					// DMGでは長さ制御(NRx1)以外を無視する
					// NRx1のDutyも無視される

					if (addr == Address::NR11 || addr == Address::NR21 || addr == Address::NR41)
					{
						apu_->writeRegister(addr, value & 0x3f);
					}
					else if (addr == Address::NR31)
					{
						apu_->writeRegister(addr, value);
					}

					return;
				}
			}

			apu_->writeRegister(addr, value);
			return;
		}
		else if (addr == Address::NR52)
		{
			apu_->writeRegister(addr, value);
			return;
		}
		else if (addr >= Address::WaveRAM && addr <= Address::WaveRAM + 15)
		{
			apu_->writeRegister(addr, value);
			return;
		}

		// LCDC, STAT, Scroll, LY, LYC
		// 0xff40 - 0xff45

		else if (addr >= Address::LCDC && addr <= Address::LYC)
		{
			lcd_->writeRegister(addr, value);
			return;
		}

		// DMA
		// 0xff46

		else if (addr == Address::DMA)
		{
			dma_.start(value);
		}

		// Palette, Window
		// 0xff47 - 0xff4b

		else if (
			(addr >= Address::BGP && addr <= Address::OBP1) ||
			(addr >= Address::WY && addr <= Address::WX))
		{
			lcd_->writeRegister(addr, value);
			return;
		}

		// (CGB) KEY1
		// 0xff4d
		// ...

		// VBK (VRAM Bank)
		// 0xff4f

		else if (addr == Address::VBK)
		{
			vramBank_ = value & 1;
			value |= 0xfe;
		}

		// Boot ROM Enable
		// 0xff50

		else if (addr == Address::BANK)
		{
			if (value & 1)
			{
				disableBootROM();
			}
		}

		// HDMA
		// 0xff51 - 0xff55

		else if (addr == Address::HDMA5)
		{
			// 仮実装
			const uint16 srcAddr = (read(Address::HDMA1) << 8) | (read(Address::HDMA2) & 0xf0);
			const uint16 dstAddr = (((read(Address::HDMA3) << 8) | read(Address::HDMA4)) & 0x1ff0) + 0x8000;
			const auto length = ((value & 0x7f) + 1) * 0x10;
			for (auto i = 0; i < length; i++)
			{
				write(dstAddr + i, read(srcAddr + i));
			}

			// FIXME: 転送モード（bit7）を考慮していないので、転送が終わったことにしている
			value = 0xff;
		}

		// RP (Infrared communications port)
		// ...

		// (CGB) Palette, OBJ priority mode
		// 0xff68 - 0xff6c
		else if (addr >= Address::BCPS && addr <= Address::OPRI)
		{
			lcd_->writeRegister(addr, value);
			return;
		}

		// WRAM Bank (SVBK)
		// 0xff70

		else if (addr == Address::SVBK)
		{
			if ((value & 0b111) == 0) value |= 1;
			wramBank_ = value & 0b111;
			value |= 0xf8;
		}

		// PCM
		// ...

		mem_[addr] = value;
	}

	void Memory::writeDirect(uint16 addr, uint8 value)
	{
		mem_[addr] = value;
	}

	uint8 Memory::read(uint16 addr) const
	{
		// ROM
		// 0x0000 - 0x7fff

		if (addr <= Address::SwitchableROMBank_End)
		{
			return mbc_->read(addr);
		}

		// VRAM
		// 0x8000 - 0x9fff

		else if (addr <= Address::VRAM_End)
		{
			return vram_[vramBank_][addr - Address::VRAM];
		}

		// SRAM
		// 0xa000 - 0xbfff

		else if (addr <= Address::SRAM_End)
		{
			return mbc_->read(addr);
		}

		// WRAM
		// 0xc000 - 0xdfff

		else if (addr <= Address::WRAM0_End)
		{
			return wram_[0][addr - Address::WRAM0];
		}
		else if (addr <= Address::WRAM1_End)
		{
			return wram_[wramBank_][addr - Address::WRAM1];
		}

		// Echo of WRAM
		// 0xe000 - 0xfdff

		else if (addr <= Address::EchoRAM_End)
		{
			return read(addr - 0x2000);
		}

		// OAM Table
		// 0xfe00 - 0xfe9f
		// DMA転送中に読むと $FF となる？

		else if (addr >= Address::OAMTable && addr <= Address::OAMTable_End)
		{
			if (dma_.running())
			{
				return 0xff;
			}
		}

		// I/O Registers
		// 0xff00 - 0xffff

		// Joypad
		// 0xff00

		else if (addr == Address::JOYP)
		{
			return joypad_->readRegister();
		}

		// Timer
		// 0xff04 - 0xff07

		else if (addr >= Address::DIV && addr <= Address::TAC)
		{
			return timer_->readRegister(addr);
		}

		// Interrupt flag (IF), Interrupt enable (IE)
		// 0xff0f, 0xffff

		else if (addr == Address::IF || addr == Address::IE)
		{
			return interrupt_->readRegister(addr);
		}

		// APU
		// 0xff10 - 0xff3f

		else if (addr >= Address::NR10 && addr < Address::NR10 + 48)
		{
			return apu_->readRegister(addr);
		}

		// LCDC, STAT, Scroll, LY, LYC
		// 0xff40 - 0xff45

		else if (addr >= Address::LCDC && addr <= Address::LYC)
		{
			return lcd_->readRegister(addr);
		}

		// Palette, Window
		// 0xff47 - 0xff4b

		else if (
			(addr >= Address::BGP && addr <= Address::OBP1) ||
			(addr >= Address::WY && addr <= Address::WX))
		{
			return lcd_->readRegister(addr);
		}

		// (CGB) Palette, OBJ priority mode
		// 0xff68 - 0xff6c
		else if (addr >= Address::BCPS && addr <= Address::OPRI)
		{
			return lcd_->readRegister(addr);
		}

		return mem_[addr];
	}

	uint8 Memory::readVRAMBank(uint16 addr, int bank) const
	{
		return vram_[bank][addr - Address::VRAM];
	}

	uint16 Memory::read16(uint16 addr) const
	{
		return read(addr) | (read(addr + 1) << 8);
	}

	uint16 Memory::read16VRAMBank(uint16 addr, int bank) const
	{
		return vram_[bank][addr - Address::VRAM] | (vram_[bank][addr + 1 - Address::VRAM] << 8);
	}

	void Memory::update(int cycles)
	{
		mbc_->update(cycles);

		dma_.update(cycles);
	}

	bool Memory::isCGBMode() const
	{
		return (static_cast<uint8>(mbc_->cgbFlag()) & 0x80) == 0x80;
	}

	int Memory::romBank() const
	{
		return mbc_->romBank();
	}

	int Memory::ramBank() const
	{
		return mbc_->ramBank();
	}

	void Memory::dumpCartridgeInfo()
	{
		mbc_->dumpCartridgeInfo();
	}

	void Memory::dump(uint16 addrBegin, uint16 addrEnd)
	{
		String dumped = U"mem({:04X}): "_fmt(addrBegin);

		for (uint16 addr = addrBegin; addr <= addrEnd; addr++)
		{
			dumped.append(U"{:02X} "_fmt(read(addr)));
		}

		DebugPrint::Writeln(dumped);
	}

	void Memory::enableBootROM(FilePathView bootROMPath)
	{
		mbc_->enableBootROM(bootROMPath);
	}

	void Memory::disableBootROM()
	{
		mbc_->disableBootROM();
	}
}
