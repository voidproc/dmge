#include "stdafx.h"
#include "MBC.h"
#include "Address.h"
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

	MBC::MBC(FilePath cartridgePath)
	{
		loadCartridge_(cartridgePath);
	}

	std::unique_ptr<MBC> MBC::LoadCartridge(FilePath cartridgePath)
	{
		const auto header = CartridgeHeader{ cartridgePath };

		if (IsNoMBC(header.type))
		{
			return std::make_unique<NoMBC>(cartridgePath);
		}
		else if (IsMBC1(header.type))
		{
			return std::make_unique<MBC1>(cartridgePath);
		}
		else if (IsMBC2(header.type))
		{
			return std::make_unique<MBC2>(cartridgePath);
		}
		else if (IsMBC3(header.type))
		{
			return std::make_unique<MBC3>(cartridgePath);
		}

		return nullptr;
	}

	void MBC::loadCartridge_(FilePath cartridgePath)
	{
		cartridgePath_ = cartridgePath;

		cartridgeHeader_ = CartridgeHeader{ cartridgePath };

		// ROMをロード
		BinaryReader reader{ cartridgePath };
		rom_.resize(reader.size());
		reader.read(rom_.data(), rom_.size());

		// SRAMをファイルからロード
		if (ramSizeBytes())
		{
			loadSRAM();
		}
	}

	void MBC::loadSRAM()
	{
		const auto savPath = FileSystem::PathAppend(FileSystem::ParentPath(cartridgePath_), FileSystem::BaseName(cartridgePath_)) + U".sav";
		if (FileSystem::Exists(savPath))
		{
			BinaryReader savReader{ savPath };
			savReader.read(sram_.data(), Min<size_t>(sram_.size(), savReader.size()));
		}
	}

	void MBC::saveSRAM()
	{
		const auto savPath = FileSystem::PathAppend(FileSystem::ParentPath(cartridgePath_), FileSystem::BaseName(cartridgePath_)) + U".sav";

		if (ramSizeBytes())
		{
			BinaryWriter writer{ savPath };
			writer.write(sram_.data(), ramSizeBytes());
		}
	}

	int MBC::romBank() const
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

	int MBC::ramBank() const
	{
		if (IsMBC1(cartridgeHeader_.type))
		{
			return bankingMode_ == 0 ? 0 : ramBank_;
		}

		return ramBank_;
	}

	int MBC::ramSizeBytes() const
	{
		if (IsMBC2(cartridgeHeader_.type))
		{
			return 512;
		}
		else
		{
			return cartridgeHeader_.ramSizeKB * 1024;
		}
	}

	void MBC::dumpCartridgeInfo()
	{
		DebugPrint::Writeln(U"* Cartridge Info:");
		DebugPrint::Writeln(U"Title={}"_fmt(cartridgeHeader_.title));

		const auto typeStr = magic_enum::enum_name<CartridgeType>(cartridgeHeader_.type);
		DebugPrint::Writeln(U"Type={}"_fmt(Unicode::WidenAscii(typeStr)));

		DebugPrint::Writeln(U"RomSize={}"_fmt(cartridgeHeader_.romSizeKB));
		DebugPrint::Writeln(U"RamSize={}"_fmt(cartridgeHeader_.ramSizeKB));
	}


	// ------------------------------------------------
	// No MBC
	// ------------------------------------------------

	void NoMBC::write(uint16, uint8)
	{
	}

	uint8 NoMBC::read(uint16 addr) const
	{
		return rom_[addr];
	}

	// ------------------------------------------------
	// MBC1
	// ------------------------------------------------

	void MBC1::write(uint16 addr, uint8 value)
	{
		if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMEnable))
		{
			// Enable RAM
			// enables external RAM if the lower 4 bits of the written value are 0xA

			ramEnabled_ = (value & 0xf) == 0xa;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBank))
		{
			// ROM Bank
			// set the ROM Bank Number

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

			ramBank_ = value & 0b11;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_BankingMode))
		{
			// MBC1:
			// Mode Select
			// set the Mode Flag to the lowest bit of the written value

			bankingMode_ = value & 1;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			// External RAM
			// write to external RAM if it is enabled. If it is not,  >>> the write is ignored <<<
			if (not ramEnabled_)
			{
				return;
			}

			const uint16 offset = addr - Address::SRAM;

			sram_[bankingMode_ == 0 ? offset : ramBank_ * 0x2000 + offset] = value;
		}
	}

	uint8 MBC1::read(uint16 addr) const
	{
		if (addr <= Address::ROMBank0_End)
		{
			// ROM Bank 0

			if (cartridgeHeader_.romSizeKB < 1024)
			{
				return rom_[addr];
			}
			else
			{
				return rom_[0x4000 * (romBank() | (ramBank_ << 5)) + addr];
			}
		}
		else if (addr <= Address::SwitchableROMBank_End)
		{
			// ROM Bank 1-
			const uint16 offset = addr - Address::SwitchableROMBank;
			return rom_[romBank() * 0x4000 + offset];
		}
		else if (addr <= Address::SRAM_End)
		{
			// External RAM

			// MBC1:
			// 大容量RAMのとき、モード1の場合、セカンダリバンクで指定されたバンクに切り替わる

			const uint16 offset = addr - Address::SRAM;
			return sram_[bankingMode_ == 0 ? offset : ramBank_ * 0x2000 + offset];
		}

		return 0;
	}

	// ------------------------------------------------
	// MBC2
	// ------------------------------------------------

	void MBC2::write(uint16 addr, uint8 value)
	{
		if (addr >= 0x0000 && addr <= 0x3fff)
		{
			if ((addr & 0x0100) == 0)
			{
				ramEnabled_ = value == 0xa;
			}
			else
			{
				value &= 0b1111;

				if (value == 0)
				{
					value = 1;
				}

				romBank_ = value;
			}
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			// External RAM
			// write to external RAM if it is enabled. If it is not,  >>> the write is ignored <<<
			if (not ramEnabled_)
			{
				return;
			}

			sram_[addr - Address::SRAM] = value;
		}
	}

	uint8 MBC2::read(uint16 addr) const
	{
		if (addr <= Address::ROMBank0_End)
		{
			// ROM Bank 0
			return rom_[addr];
		}
		else if (addr <= Address::SwitchableROMBank_End)
		{
			// ROM Bank 1-
			const uint16 offset = addr - Address::SwitchableROMBank;
			return rom_[romBank() * 0x4000 + offset];
		}
		else if (addr <= Address::SRAM_End)
		{
			// Built in RAM
			return sram_[(addr - Address::SRAM) & 0x1ff];
		}

		return 0;
	}

	// ------------------------------------------------
	// MBC3
	// ------------------------------------------------

	void MBC3::write(uint16 addr, uint8 value)
	{
		if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMEnable))
		{
			MBC1::write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBank))
		{
			// ROM Bank
			// set the ROM Bank Number

			value &= 0b01111111;

			if (value == 0)
			{
				value = 1;
			}

			romBank_ = value;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMBank))
		{
			MBC1::write(addr, value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			// External RAM
			// write to external RAM if it is enabled. If it is not,  >>> the write is ignored <<<
			if (not ramEnabled_)
			{
				return;
			}

			const uint16 offset = addr - Address::SRAM;
			sram_[ramBank_ * 0x2000 + offset] = value;
		}
	}

	uint8 MBC3::read(uint16 addr) const
	{
		if (addr <= Address::ROMBank0_End)
		{
			// ROM Bank 0
			return rom_[addr];
		}
		else if (addr <= Address::SwitchableROMBank_End)
		{
			// ROM Bank 1-
			return MBC1::read(addr);
		}
		else if (addr <= Address::SRAM_End)
		{
			// External RAM

			const uint16 offset = addr - Address::SRAM;
			return sram_[ramBank_ * 0x2000 + offset];
		}

		return 0;
	}
}
