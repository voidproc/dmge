#include "Memory.h"
#include "MBC.h"
#include "PPU.h"
#include "LCD.h"
#include "Audio/APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "Serial.h"
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

	void Memory::init(PPU* ppu, APU* apu, dmge::Timer* timer, dmge::Joypad* joypad, LCD* lcd, Interrupt* interrupt, Serial* serial)
	{
		ppu_ = ppu;
		apu_ = apu;
		timer_ = timer;
		joypad_ = joypad;
		serial_ = serial;
		lcd_ = lcd;
		interrupt_ = interrupt;
		sgbPacket_ = std::make_unique<SGB::PacketTransfer>(*joypad_);
	}

	void Memory::reset()
	{
		writeDirect(0xff00, 0xcf); // P1
		writeDirect(0xff01, 0x00); // SB
		writeDirect(0xff02, isCGBMode() ? 0x7f : 0x7e); // SC

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
		write(0xff26, isSGBMode() ? 0xf0 : 0xf1); // NR52 (GB:0xf1, SGB:0xf0)
		write(0xff40, 0x91); // LCDC
		write(0xff42, 0x00); // SCY
		write(0xff43, 0x00); // SCX
		write(0xff45, 0x00); // LYC
		writeDirect(0xff46, isCGBMode() ? 0x00 : 0xff); // DMA
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

		if (addr <= Address::SwitchableROMBank_End)
		{
			// MBC
			// 0x0000 - 0x7fff

			mbc_->write(addr, value);
		}
		else if (addr <= Address::VRAM_End)
		{
			// VRAM
			// 0x8000 - 0x9fff

			vram_[vramBank_][addr - Address::VRAM] = value;

			vramTileDataModified_ = true;
		}
		else if (addr <= Address::SRAM_End)
		{
			// SRAM
			// 0xa000 - 0xbfff

			mbc_->write(addr, value);
		}
		else if (addr <= Address::WRAM0_End)
		{
			// WRAM bank 0
			// 0xc000 - 0xcfff

			wram_[0][addr - Address::WRAM0] = value;
		}
		else if (addr <= Address::WRAM1_End)
		{
			// WRAM switchable
			// 0xd000 - 0xdfff

			wram_[wramBank_][addr - Address::WRAM1] = value;
		}
		else if (addr <= Address::EchoRAM_End)
		{
			// Echo of WRAM
			// 0xe000 - 0xfdff

			write(addr - 0x2000, value);
		}
		else if (addr <= Address::OAMTable_End)
		{
			// OAM Table
			// 0xfe00 - 0xfe9f
			// ...

			goto Fallback_WriteToMemory;
		}
		else if (addr <= 0xfeff)
		{
			// Not usable
			// 0xfea0 - 0xfeff

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::JOYP)
		{
			// Joypad
			// 0xff00

			joypad_->writeRegister(value);

			if (isSGBMode())
			{
				sgbPacket_->send((value >> 4) & 0b11);
			}
		}
		else if (addr <= Address::SC)
		{
			// Serial
			// 0xff01 - 0xff02

			serial_->writeRegister(addr, value);
		}
		else if (addr <= 0xff03)
		{
			// ?
			// 0xff03

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::TAC)
		{
			// Timer
			// 0xff04 - 0xff07

			timer_->writeRegister(addr, value);
		}
		else if (addr <= 0xff0e)
		{
			// ?
			// 0xff08 - 0xff0e

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::IF)
		{
			// Interrupt Flag (IF)
			// 0xff0f

			interrupt_->writeRegister(addr, value);
		}
		else if (addr <= Address::NR51)
		{
			// APU
			// 0xff10 - 0xff25

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
		}
		else if (addr <= Address::WaveRAM_End)
		{
			// APU master switch ~ wave RAM
			// 0xff26 - 0xff3f

			apu_->writeRegister(addr, value);
		}
		else if (addr <= Address::LYC)
		{
			// LCDC, STAT, Scroll, LY, LYC
			// 0xff40 - 0xff45

			lcd_->writeRegister(addr, value);
		}
		else if (addr <= Address::DMA)
		{
			// DMA
			// 0xff46

			dma_.start(value);
			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::WX)
		{
			// Palette (BGP, OBP0/1), Window (WY, WX)
			// 0xff47 - 0xff4b

			lcd_->writeRegister(addr, value);
		}
		else if (addr <= 0xff4c)
		{
			// ?
			// 0xff4c

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::KEY1)
		{
			// (CGB) KEY1
			// 0xff4d

			if (value & 1)
			{
				doubleSpeedPrepared_ = true;
			}

			goto Fallback_WriteToMemory;
		}
		else if (addr <= 0xff4e)
		{
			// ?
			// 0xff4e

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::VBK)
		{
			// VBK (VRAM Bank)
			// 0xff4f

			vramBank_ = value & 1;
			value |= 0xfe;
			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::BANK)
		{
			// Boot ROM Enable
			// 0xff50

			if (value & 1)
			{
				disableBootROM();
			}

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::HDMA4)
		{
			// HDMA アドレス指定
			// 0xff51 - 0xff54

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::HDMA5)
		{
			// HDMA 制御
			// 0xff55

			// 仮実装

			// HBlank DMA が開始され、その後 Bit7 がリセットされたときなにもしないで終了する（暫定）
			{
				if (value & 0x80)
				{
					hblankDMATransferring_ = true;
				}

				if (hblankDMATransferring_)
				{
					if ((value & 0x80) == 0)
					{
						hblankDMATransferring_ = false;
						goto Fallback_WriteToMemory;
					}
				}
			}

			const uint16 srcAddr = (read(Address::HDMA1) << 8) | (read(Address::HDMA2) & 0xf0);
			const uint16 dstAddr = (((read(Address::HDMA3) << 8) | read(Address::HDMA4)) & 0x1ff0) + 0x8000;
			const auto length = ((value & 0x7f) + 1) * 0x10;
			for (auto i = 0; i < length; i++)
			{
				write(dstAddr + i, read(srcAddr + i));
			}

			// 転送が一瞬で終わったことにする
			value = 0xff;

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::RP)
		{
			// RP (Infrared communications port)
			// 0xff56
			// ...

			goto Fallback_WriteToMemory;
		}
		else if (addr <= 0xff67)
		{
			// ?
			// 0xff57 - 0xff67

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::OPRI)
		{
			// (CGB) Palette, OBJ priority mode
			// 0xff68 - 0xff6c

			lcd_->writeRegister(addr, value);
		}
		else if (addr <= 0xff6f)
		{
			// ?
			// 0xff6d - 0xff6f

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::SVBK)
		{
			// WRAM Bank (SVBK)
			// 0xff70

			if ((value & 0b111) == 0) value |= 1;
			wramBank_ = value & 0b111;
			value |= 0xf8;

			goto Fallback_WriteToMemory;
		}
		else if (addr <= 0xff75)
		{
			// ?
			// 0xff71 - 0xff75

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::PCM34)
		{
			// PCM12, PCM34
			// 0xff76 - 0xff77
			// ...

			goto Fallback_WriteToMemory;
		}
		else if (addr <= 0xff7f)
		{
			// ?
			// 0xff78 - 0xff7f

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::HRAM_End)
		{
			// HRAM
			// 0xff80 - 0xfffe
			//...

			goto Fallback_WriteToMemory;
		}
		else if (addr <= Address::IE)
		{
			// Interrupt enable (IE)
			// 0xffff

			interrupt_->writeRegister(addr, value);
		}

		return;

	Fallback_WriteToMemory:
		mem_[addr] = value;
	}

	void Memory::writeDirect(uint16 addr, uint8 value)
	{
		mem_[addr] = value;
	}

	uint8 Memory::read(uint16 addr) const
	{
		if (addr <= Address::SwitchableROMBank_End)
		{
			// MBC
			// 0x0000 - 0x7fff

			return mbc_->read(addr);
		}
		else if (addr <= Address::VRAM_End)
		{
			// VRAM
			// 0x8000 - 0x9fff

			return vram_[vramBank_][addr - Address::VRAM];
		}
		else if (addr <= Address::SRAM_End)
		{
			// SRAM
			// 0xa000 - 0xbfff

			return mbc_->read(addr);
		}
		else if (addr <= Address::WRAM0_End)
		{
			// WRAM bank 0
			// 0xc000 - 0xcfff

			return wram_[0][addr - Address::WRAM0];
		}
		else if (addr <= Address::WRAM1_End)
		{
			// WRAM switchable
			// 0xd000 - 0xdfff

			return wram_[wramBank_][addr - Address::WRAM1];
		}
		else if (addr <= Address::EchoRAM_End)
		{
			// Echo of WRAM
			// 0xe000 - 0xfdff

			return read(addr - 0x2000);
		}
		else if (addr <= Address::OAMTable_End)
		{
			// OAM Table
			// 0xfe00 - 0xfe9f

			// DMA転送中に読むと $FF となる？
			if (dma_.running())
			{
				return 0xff;
			}
		}
		else if (addr <= 0xfeff)
		{
			// Not usable
			// 0xfea0 - 0xfeff
		}
		else if (addr == Address::JOYP)
		{
			// Joypad
			// 0xff00

			return joypad_->readRegister();
		}
		else if (addr <= Address::SC)
		{
			// Serial
			// 0xff01 - 0xff02

			return serial_->readRegister(addr);
		}
		else if (addr <= 0xff03)
		{
			// ?
			// 0xff03
		}
		else if (addr <= Address::TAC)
		{
			// Timer
			// 0xff04 - 0xff07

			return timer_->readRegister(addr);
		}
		else if (addr <= 0xff0e)
		{
			// ?
			// 0xff08 - 0xff0e
		}
		else if (addr == Address::IF)
		{
			// Interrupt flag (IF)
			// 0xff0f

			return interrupt_->readRegister(addr);
		}
		else if (addr <= Address::WaveRAM_End)
		{
			// APU
			// 0xff10 - 0xff3f

			return apu_->readRegister(addr);
		}
		else if (addr <= Address::LYC)
		{
			// LCDC, STAT, Scroll, LY, LYC
			// 0xff40 - 0xff45

			return lcd_->readRegister(addr);
		}
		else if (addr <= Address::DMA)
		{
			// DMA
			// 0xff46
			//...
		}
		else if (addr <= Address::WX)
		{
			// Palette, Window
			// 0xff47 - 0xff4b

			return lcd_->readRegister(addr);
		}
		else if (addr <= 0xff4c)
		{
			// ?
			// 0xff4c
		}
		else if (addr <= Address::KEY1)
		{
			// (CGB) KEY1
			// 0xff4d

			return ((uint8)doubleSpeed_ << 7) | ((uint8)doubleSpeedPrepared_);
		}
		else if (addr <= 0xff4e)
		{
			// ?
			// 0xff4e
		}
		else if (addr <= Address::VBK)
		{
			// VBK (VRAM Bank)
			// 0xff4f
		}
		else if (addr <= Address::BANK)
		{
			// Boot ROM Enable
			// 0xff50
		}
		else if (addr <= Address::HDMA5)
		{
			// HDMA
			// 0xff51 - 0xff55
		}
		else if (addr <= Address::RP)
		{
			// RP (Infrared communications port)
			// 0xff56
			// ...
		}
		else if (addr <= 0xff67)
		{
			// ?
			// 0xff57 - 0xff67
		}
		else if (addr <= Address::OPRI)
		{
			// (CGB) Palette, OBJ priority mode
			// 0xff68 - 0xff6c

			return lcd_->readRegister(addr);
		}
		else if (addr <= 0xff6f)
		{
			// ?
			// 0xff6d - 0xff6f
		}
		else if (addr <= Address::SVBK)
		{
			// WRAM Bank (SVBK)
			// 0xff70
		}
		else if (addr <= 0xff75)
		{
			// ?
			// 0xff71 - 0xff75
		}
		else if (addr <= Address::PCM34)
		{
			// PCM12, PCM34
			// 0xff76 - 0xff77
			// ...
		}
		else if (addr <= 0xff7f)
		{
			// ?
			// 0xff78 - 0xff7f
		}
		else if (addr <= Address::HRAM_End)
		{
			// HRAM
			// 0xff80 - 0xfffe
			//...
		}
		else if (addr == Address::IE)
		{
			// Interrupt enable (IE)
			// 0xffff

			return interrupt_->readRegister(addr);
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

	bool Memory::isSupportedCGBMode() const
	{
		return (static_cast<uint8>(mbc_->cgbFlag()) & 0x80) == 0x80;
	}

	bool Memory::isSupportedSGBMode() const
	{
		return static_cast<uint8>(mbc_->sgbFlag()) == 0x03;
	}

	void Memory::setCGBMode(bool enableCGBMode)
	{
		cgbMode_ = enableCGBMode;
	}

	bool Memory::isCGBMode() const
	{
		return cgbMode_;
	}

	void Memory::setSGBMode(bool enableSGBMode)
	{
		sgbMode_ = enableSGBMode;
	}

	bool Memory::isSGBMode() const
	{
		return sgbMode_;
	}

	int Memory::romBank() const
	{
		return mbc_->romBank();
	}

	int Memory::ramBank() const
	{
		return mbc_->ramBank();
	}

	int Memory::vramBank() const
	{
		return vramBank_;
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

	bool Memory::isVRAMTileDataModified()
	{
		return vramTileDataModified_;
	}

	void Memory::resetVRAMTileDataModified()
	{
		vramTileDataModified_ = false;
	}

	void Memory::switchDoubleSpeed()
	{
		if (doubleSpeedPrepared_)
		{
			doubleSpeed_ = not doubleSpeed_;
			doubleSpeedPrepared_ = false;

			timer_->resetDIV();

			apu_->setDoubleSpeed(doubleSpeed_);
		}
	}

	bool Memory::isDoubleSpeed() const
	{
		return doubleSpeed_;
	}
}
