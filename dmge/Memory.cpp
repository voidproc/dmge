#include "Memory.h"
#include "PPU.h"
#include "APU.h"
#include "Timer.h"
#include "Joypad.h"
#include "DebugPrint.h"
#include "lib/magic_enum/magic_enum.hpp"

namespace dmge
{
	bool IsNoMBC(CartridgeType type)
	{
		return type == CartridgeType::ROM_ONLY;
	}

	bool IsMBC1(CartridgeType type)
	{
		switch (type)
		{
			case CartridgeType::MBC1:
			case CartridgeType::MBC1_RAM:
			case CartridgeType::MBC1_RAM_BATTERY:
				return true;
		}

		return false;
	}

	bool IsMBC2(CartridgeType type)
	{
		switch (type)
		{
		case CartridgeType::MBC2:
		case CartridgeType::MBC2_BATTERY:
			return true;
		}

		return false;
	}

	bool IsMBC3(CartridgeType type)
	{
		switch (type)
		{
		case CartridgeType::MBC3_TIMER_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY_2:
		case CartridgeType::MBC3:
		case CartridgeType::MBC3_RAM_2:
		case CartridgeType::MBC3_RAM_BATTERY_2:
			return true;
		}

		return false;
	}

	Memory::Memory()
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

	void Memory::loadCartridge(FilePath cartridgePath)
	{
		cartridgePath_ = cartridgePath;

		// ROMをロード
		BinaryReader reader{ cartridgePath };
		rom_.resize(reader.size());
		reader.read(rom_.data(), rom_.size());

		// メモリにROMデータ書き込み
		mem_.resize(0x10000);
		std::copy_n(rom_.cbegin(), 0x8000, mem_.begin());
		//reader.setPos(0);
		//reader.read(mem_.data(), 0x8000);

		// ヘッダ読み込み
		readCartridgeHeader_();

		// SRAM (32KiB)
		sram_.resize(0x8000);

		// SRAMをファイルからロード
		if (cartridgeHeader_.ramSizeKB)
		{
			const auto savPath = FileSystem::PathAppend(FileSystem::ParentPath(cartridgePath), FileSystem::BaseName(cartridgePath)) + U".sav";
			if (FileSystem::Exists(savPath))
			{
				BinaryReader savReader{ savPath };
				savReader.read(sram_.data(), Min<size_t>(sram_.size(), savReader.size()));
			}
		}

		// メモリを初期状態にリセット
		reset();
	}

	void Memory::saveSRAM()
	{
		const auto savPath = FileSystem::PathAppend(FileSystem::ParentPath(cartridgePath_), FileSystem::BaseName(cartridgePath_)) + U".sav";

		if (cartridgeHeader_.ramSizeKB)
		{
			BinaryWriter writer{ savPath };
			writer.write(sram_.data(), cartridgeHeader_.ramSizeKB * 1024);
		}
	}

	void Memory::write(uint16 addr, uint8 value)
	{
		bool doWrite = true;

		// MBC

		if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMEnable))
		{
			// Enable RAM
			// enables external RAM if the lower 4 bits of the written value are 0xA

			doWrite = false;

			ramEnabled_ = (value & 0xf) == 0xa;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBank))
		{
			// ROM Bank
			// set the ROM Bank Number

			doWrite = false;

			if (IsMBC1(cartridgeHeader_.type))
			{
				switch (cartridgeHeader_.romSizeKB)
				{
				case 32:   value &= 0b00000001; break;
				case 64:   value &= 0b00000011; break;
				case 128:  value &= 0b00000111; break;
				case 256:  value &= 0b00001111; break;
				case 512:  value &= 0b00011111; break;
				case 1024: value &= 0b00011111; break;
				case 2048: value &= 0b00011111; break;
				}
			}
			else if (IsMBC3(cartridgeHeader_.type))
			{
				value &= 0b01111111;
			}

			if (value == 0)
			{
				value = 1;
			}

			romBank_ = value;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMBank))
		{
			// RAM Bank
			// set the 2 bits of the RAM bank number to the lowest 2 bits of the written value

			doWrite = false;

			ramBank_ = value & 0b11;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_BankingMode))
		{
			doWrite = false;

			// MBC1:
			// Mode Select
			// set the Mode Flag to the lowest bit of the written value

			if (IsMBC1(cartridgeHeader_.type))
			{
				bankingMode_ = value & 1;
			}
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			doWrite = false;

			// External RAM
			// write to external RAM if it is enabled. If it is not,  >>> the write is ignored <<<
			if (not ramEnabled_)
			{
				goto HOOK;
			}

			const uint16 offset = addr - Address::SRAM;

			if (IsMBC1(cartridgeHeader_.type))
			{
				sram_[bankingMode_ == 0 ? offset : ramBank_ * 0x2000 + offset] = value;
			}
			else
			{
				sram_[ramBank_ * 0x2000 + offset] = value;
			}
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

		if (not writeHooks_.empty())
		{
			for (const auto& hook : writeHooks_)
			{
				hook(addr, value);
			}
		}

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
		uint8 value = mem_[addr];

		// MBC

		if (ADDRESS_IN_RANGE(addr, Address::ROMBank0))
		{
			// ROM Bank 0

			// MBC1
			// 大容量ROMのとき、モード1の場合、ROMバンク番号の上位2bitがセカンダリバンクの影響を受ける
			if (IsMBC1(cartridgeHeader_.type))
			{
				if (cartridgeHeader_.romSizeKB >= 1024)
				{
					value = rom_[0x4000 * (romBank() | (ramBank_ << 5)) + addr];
				}
			}
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SwitchableROMBank))
		{
			// ROM Bank 1-
			const uint16 offset = addr - Address::SwitchableROMBank;
			value = rom_[romBank() * 0x4000 + offset];
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			// External RAM

			// MBC1:
			// 大容量RAMのとき、モード1の場合、セカンダリバンクで指定されたバンクに切り替わる

			const uint16 offset = addr - Address::SRAM;

			if (IsMBC1(cartridgeHeader_.type))
			{
				value = sram_[bankingMode_ == 0 ? offset : ramBank_ * 0x2000 + offset];
			}
			else
			{
				value = sram_[ramBank_ * 0x2000 + offset];
			}
		}

		return value;
	}

	uint16 Memory::read16(uint16 addr) const
	{
		return read(addr) | (read(addr + 1) << 8);
	}

	int Memory::romBank() const
	{
		if (IsMBC1(cartridgeHeader_.type))
		{
			if (cartridgeHeader_.romSizeKB >= 1024)
			{
				return romBank_ + (ramBank_ << 5);
			}
		}

		return romBank_;
	}

	int Memory::ramBank() const
	{
		if (IsMBC1(cartridgeHeader_.type))
		{
			return bankingMode_ == 0 ? 0 : ramBank_;
		}

		return ramBank_;
	}

	void Memory::dumpCartridgeInfo()
	{
		DebugPrint::Writeln(U"* Cartridge Info:");
		DebugPrint::Writeln(U"Title={}"_fmt(cartridgeHeader_.title));

		const auto typeStr = magic_enum::enum_name<CartridgeType>(cartridgeHeader_.type);
		DebugPrint::Writeln(U"Type={}"_fmt(Unicode::WidenAscii(typeStr)));

		DebugPrint::Writeln(U"RomSize={}"_fmt(cartridgeHeader_.romSizeKB));
		DebugPrint::Writeln(U"RamSize={}"_fmt(cartridgeHeader_.ramSizeKB));
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

	void Memory::readCartridgeHeader_()
	{
		// Title
		cartridgeHeader_.title = rom_.slice(Address::Title, 15).map([](uint8 x) { return static_cast<char32_t>(x); }).join(U""_sv, U""_sv, U""_sv);

		// Type
		cartridgeHeader_.type = static_cast<CartridgeType>(rom_[Address::CartridgeType]);

		// ROM Size

		const uint8 romSize = rom_[Address::ROMSize];
		cartridgeHeader_.romSizeKB = 32 * (1 << romSize);

		// RAM Size

		const uint8 ramSize = rom_[Address::RAMSize];
		int ramSizeKB = 0;

		switch (ramSize)
		{
		case 0: ramSizeKB = 0; break;
		case 1: ramSizeKB = 0; break;
		case 2: ramSizeKB = 8; break;
		case 3: ramSizeKB = 32; break;
		case 4: ramSizeKB = 128; break;
		case 5: ramSizeKB = 64; break;
		}

		cartridgeHeader_.ramSizeKB = ramSizeKB;
	}
}
