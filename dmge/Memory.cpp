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
		write(0xff05, 0x00); // TIMA
		write(0xff06, 0x00); // TMA
		writeDirect(0xff07, 0xf8); // TAC, 0xff80 | (0x00 & ~0xff80)
		writeDirect(0xff0f, 0xe0); // IF
		write(0xff10, 0x80); // NR10
		write(0xff11, 0xbf); // NR11
		write(0xff12, 0xf3); // NR12
		write(0xff14, 0xbf); // NR14
		write(0xff16, 0x3f); // NR21
		write(0xff17, 0x00); // NR22
		write(0xff19, 0xbf); // NR24
		write(0xff1a, 0x7f); // NR30
		write(0xff1b, 0xff); // NR31
		write(0xff1c, 0x9f); // NR32
		write(0xff1e, 0xbf); // NR33
		write(0xff20, 0xff); // NR41
		write(0xff21, 0x00); // NR42
		write(0xff22, 0x00); // NR43
		write(0xff23, 0xbf); // NR30
		write(0xff24, 0x77); // NR50
		write(0xff25, 0xf3); // NR51
		write(0xff26, 0xf1); // NR52 (GB:0xf1, SGB:0xf0)
		write(0xff40, 0x91); // LCDC
		write(0xff42, 0x00); // SCY
		write(0xff43, 0x00); // SCX
		write(0xff45, 0x00); // LYC
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

		// メモリ
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
		bool doWrite = true;

		// MBC

		if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMEnable))
		{
			doWrite = false;
			mbc_->write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBank))
		{
			doWrite = false;
			mbc_->write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMBank))
		{
			doWrite = false;
			mbc_->write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_BankingMode))
		{
			doWrite = false;
			mbc_->write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			doWrite = false;
			mbc_->write(addr, value);
		}

		// Joypad

		else if (addr == Address::JOYP)
		{
			joypad_->update(value);
			doWrite = false;
		}

		// Timer

		else if (addr == Address::DIV)
		{
			timer_->resetInternalDIV();
			doWrite = false;
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
		else if (addr == Address::NR42)
		{
			apu_->setEnvelopeAndDAC(Channels::Ch4, value);
		}

		// APU - Update Frequency

		else if (addr == Address::NR13)
		{
			const int freq = value | ((read(Address::NR14) & 0b111) << 8);
			apu_->setFrequency(Channels::Ch1, freq);
		}
		else if (addr == Address::NR23)
		{
			const int freq = value | ((read(Address::NR24) & 0b111) << 8);
			apu_->setFrequency(Channels::Ch2, freq);
		}
		else if (addr == Address::NR33)
		{
			const int freq = value | ((read(Address::NR34) & 0b111) << 8);
			apu_->setFrequency(Channels::Ch3, freq);
		}

		// APU - Channel Trigger (Update Frequency)

		else if (addr == Address::NR14)
		{
			apu_->trigger(Channels::Ch1);
		}
		else if (addr == Address::NR24)
		{
			apu_->trigger(Channels::Ch2);
		}
		else if (addr == Address::NR34)
		{
			apu_->trigger(Channels::Ch3);
		}
		else if (addr == Address::NR44)
		{
			apu_->trigger(Channels::Ch4);
		}

		// DMA

		else if (addr == Address::DMA)
		{
			uint16 srcAddr = value * 0x100;
			for (uint16 offset = 0; offset <= 0x9f; offset++)
			{
				mem_[0xfe00 + offset] = read(srcAddr + offset);
			}
		}

		// Boot ROM

		else if (addr == Address::BANK)
		{
			// Enable Boot ROM
			//...
		}


	HOOK:

		// フック

		//if (not writeHooks_.empty())
		//{
		//	for (const auto& hook : writeHooks_)
		//	{
		//		hook(addr, value);
		//	}
		//}

		if (doWrite)
		{
			mem_[addr] = value;
		}
	}

	void Memory::writeDirect(uint16 addr, uint8 value)
	{
		mem_[addr] = value;
	}

	uint8 Memory::read(uint16 addr) const
	{
		uint8 value;

		if (ADDRESS_IN_RANGE(addr, Address::ROMBank0))
		{
			// ROM Bank 0
			value = mbc_->read(addr);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SwitchableROMBank))
		{
			// ROM Bank 1-
			value = mbc_->read(addr);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			// External RAM
			value = mbc_->read(addr);
		}
		else
		{
			value = mem_[addr];
		}

		return value;
	}

	uint16 Memory::read16(uint16 addr) const
	{
		return read(addr) | (read(addr + 1) << 8);
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
