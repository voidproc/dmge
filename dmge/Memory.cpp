#include "Memory.h"
#include "MBC.h"
#include "PPU.h"
#include "APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "DebugPrint.h"

namespace dmge
{
	Memory::Memory()
	{
	}

	Memory::~Memory()
	{
	}

	void Memory::init(PPU* ppu, APU* apu, dmge::Timer* timer, dmge::Joypad* joypad)
	{
		ppu_ = ppu;
		apu_ = apu;
		timer_ = timer;
		joypad_ = joypad;
	}

	void Memory::reset()
	{
		const bool cgb = isCGBMode();

		writeDirect(0xff00, 0xcf); // P1
		writeDirect(0xff01, 0x00); // SB
		writeDirect(0xff02, cgb ? 0x7f : 0x7e); // SC

		writeDirect(0xff05, 0x00); // TIMA
		writeDirect(0xff06, 0x00); // TMA
		writeDirect(0xff07, 0xf8); // TAC, 0xff80 | (0x00 & ~0xff80)
		writeDirect(0xff0f, 0xe0); // IF
		writeDirect(0xff10, 0x80); // NR10
		writeDirect(0xff11, 0xbf); // NR11
		writeDirect(0xff12, 0xf3); // NR12
		writeDirect(0xff13, 0xff); // NR13
		writeDirect(0xff14, 0xbf); // NR14
		writeDirect(0xff16, 0x3f); // NR21
		writeDirect(0xff17, 0x00); // NR22
		writeDirect(0xff18, 0xff); // NR23
		writeDirect(0xff19, 0xbf); // NR24
		writeDirect(0xff1a, 0x7f); // NR30
		writeDirect(0xff1b, 0xff); // NR31
		writeDirect(0xff1c, 0x9f); // NR32
		writeDirect(0xff1e, 0xff); // NR33
		writeDirect(0xff1f, 0xbf); // NR34
		writeDirect(0xff20, 0xff); // NR41
		writeDirect(0xff21, 0x00); // NR42
		writeDirect(0xff22, 0x00); // NR43
		writeDirect(0xff23, 0xbf); // NR44
		writeDirect(0xff24, 0x77); // NR50
		writeDirect(0xff25, 0xf3); // NR51
		writeDirect(0xff26, 0xf1); // NR52 (GB:0xf1, SGB:0xf0)
		writeDirect(0xff40, 0x91); // LCDC
		writeDirect(0xff42, 0x00); // SCY
		writeDirect(0xff43, 0x00); // SCX
		writeDirect(0xff45, 0x00); // LYC
		writeDirect(0xff46, cgb ? 0x00 : 0xff); // DMA
		writeDirect(0xff47, 0xfc); // BGP
		writeDirect(0xff48, 0xff); // OBP0
		writeDirect(0xff49, 0xff); // OBP1
		writeDirect(0xff4a, 0x00); // WY
		writeDirect(0xff4b, 0x00); // WX
		writeDirect(0xffff, 0x00); // IE

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

		if (addr <= Address::MBC_BankingMode_End)
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

		// IO-Reg.
		// 0xff00 - 0xffff

		// Joypad

		else if (addr == Address::JOYP)
		{
			joypad_->update(value);
			return;
		}

		// Timer

		else if (addr == Address::DIV)
		{
			timer_->resetInternalDIV();
			return;
		}
		else if (addr == Address::TIMA)
		{
			timer_->abortInterrupt();
		}
		else if (addr == Address::TAC)
		{
			value = 0xf8 | (value & 0x07);
		}

		// IF (Interrupt Flag)

		else if (addr == Address::IF)
		{
			value |= 0b11100000;
		}

		// APU - Ignore if APU is off

		else if (addr >= Address::NR10 && addr <= Address::NR51 && (read(Address::NR52) & 0x80) == 0)
		{
			return;
		}

		// APU - Reload Length Timer

		else if (addr == Address::NR11)
		{
			apu_->setLengthTimer(Channels::Ch1, value);
		}
		else if (addr == Address::NR21)
		{
			apu_->setLengthTimer(Channels::Ch2, value);
		}
		else if (addr == Address::NR31)
		{
			apu_->setLengthTimer(Channels::Ch3, value);
		}
		else if (addr == Address::NR41)
		{
			apu_->setLengthTimer(Channels::Ch4, value);
		}

		// APU - Enable DAC / Envelope ?

		else if (addr == Address::NR12)
		{
			apu_->setEnvelopeAndDAC(Channels::Ch1, value);
		}
		else if (addr == Address::NR22)
		{
			apu_->setEnvelopeAndDAC(Channels::Ch2, value);
		}
		else if (addr == Address::NR30)
		{
			apu_->setDAC(Channels::Ch3, (value >> 7) == 1);
		}
		else if (addr == Address::NR42)
		{
			apu_->setEnvelopeAndDAC(Channels::Ch4, value);
		}

		// APU - Update Frequency

		else if (addr == Address::NR13)
		{
			apu_->setFrequencyLow(Channels::Ch1, value);
		}
		else if (addr == Address::NR23)
		{
			apu_->setFrequencyLow(Channels::Ch2, value);
		}
		else if (addr == Address::NR33)
		{
			apu_->setFrequencyLow(Channels::Ch3, value);
		}

		// APU - Channel Trigger (Update Frequency)

		else if (addr == Address::NR14)
		{
			apu_->setFrequencyHigh(Channels::Ch1, value);

			if (value & 0x80)
				apu_->trigger(Channels::Ch1);
		}
		else if (addr == Address::NR24)
		{
			apu_->setFrequencyHigh(Channels::Ch2, value);

			if (value & 0x80)
				apu_->trigger(Channels::Ch2);
		}
		else if (addr == Address::NR34)
		{
			apu_->setFrequencyHigh(Channels::Ch3, value);

			if (value & 0x80)
				apu_->trigger(Channels::Ch3);
		}
		else if (addr == Address::NR44)
		{
			if (value & 0x80)
				apu_->trigger(Channels::Ch4);
		}

		// APU - NR52

		else if (addr == Address::NR52)
		{
			apu_->setEnable(Channels::Ch1, ((value >> 0) & 1) == 1);
			apu_->setEnable(Channels::Ch2, ((value >> 1) & 1) == 1);
			apu_->setEnable(Channels::Ch3, ((value >> 2) & 1) == 1);
			apu_->setEnable(Channels::Ch4, ((value >> 3) & 1) == 1);

			// APUがoffになったとき、APUレジスタが全てクリアされる
			if ((value & 0x80) == 0)
			{
				for (uint16 apuReg = Address::NR10; apuReg <= Address::NR51; apuReg++)
				{
					write(apuReg, 0);
				}
			}

			value &= 0x80;
		}

		// DMA (0xff46)

		else if (addr == Address::DMA)
		{
			uint16 srcAddr = value * 0x100;
			for (uint16 offset = 0; offset <= 0x9f; offset++)
			{
				mem_[0xfe00 + offset] = read(srcAddr + offset);
			}
		}

		// VRAM Bank (VBK) (0xff4f)

		else if (addr == Address::VBK)
		{
			vramBank_ = value & 1;
			value |= 0xfe;
		}

		// HDMA (0xff51 - 0xff55)

		else if (addr == Address::HDMA5)
		{
			const uint16 srcAddr = (read(Address::HDMA1) << 8) | (read(Address::HDMA2) & 0xf0);
			const uint16 dstAddr = (((read(Address::HDMA3) << 8) | read(Address::HDMA4)) & 0x1ff0) + 0x8000;
			const auto length = ((value & 0x7f) + 1) * 0x10;
			for (auto i = 0; i < length; i++)
			{
				write(dstAddr + i, read(srcAddr + i));
			}

			// TODO: 転送モード（bit7）を考慮していないので、転送が終わったことにする
			value = 0xff;
		}

		// BCPS/BGPI (0xff68)

		if (addr == Address::BCPS)
		{
			ppu_->setBGPaletteIndex(value & 0x3f, value >> 7);
		}

		// BCPD/BGPD (0xff69)

		if (addr == Address::BCPD)
		{
			ppu_->setBGPaletteData(value);
		}

		// OCPS/OBPI (0xff6a)

		if (addr == Address::OCPS)
		{
			ppu_->setOBJPaletteIndex(value & 0x3f, value >> 7);
		}

		// OCPD/OBPD (0xff6b)

		if (addr == Address::OCPD)
		{
			ppu_->setOBJPaletteData(value);
		}

		// WRAM Bank (SVBK) (0xff70)

		else if (addr == Address::SVBK)
		{
			if ((value & 0b111) == 0) value |= 1;
			wramBank_ = value & 0b111;
			value |= 0xf8;
		}

		// Boot ROM

		else if (addr == Address::BANK)
		{
			// Enable Boot ROM
			//...
		}

		mem_[addr] = value;
	}

	void Memory::writeDirect(uint16 addr, uint8 value)
	{
		mem_[addr] = value;
	}

	uint8 Memory::read(uint16 addr) const
	{
		// APU

		if (addr >= Address::NR10 && addr < Address::NR10 + 48)
		{
			static constexpr std::array<uint8, 48> mask = {
				0x80,0x3F,0x00,0xFF,0xBF,
				0xFF,0x3F,0x00,0xFF,0xBF,
				0x7F,0xFF,0x9F,0xFF,0xBF,
				0xFF,0xFF,0x00,0x00,0xBF,
				0x00,0x00,0x70,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			};
			return mem_[addr] | mask[addr - Address::NR10];
		}

		// Others

		if (addr > Address::WRAM1_End)
		{
			return mem_[addr];
		}

		// ROM

		else if (addr <= Address::SwitchableROMBank_End)
		{
			return mbc_->read(addr);
		}

		// VRAM

		else if (addr <= Address::VRAM_End)
		{
			return vram_[vramBank_][addr - Address::VRAM];
		}

		// SRAM

		else if (addr <= Address::SRAM_End)
		{
			return mbc_->read(addr);
		}

		// WRAM

		else if (addr <= Address::WRAM0_End)
		{
			return wram_[0][addr - Address::WRAM0];
		}
		else if (addr <= Address::WRAM1_End)
		{
			return wram_[wramBank_][addr - Address::WRAM1];
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
}
