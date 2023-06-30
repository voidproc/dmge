#include "stdafx.h"
#include "MBC.h"
#include "Address.h"
#include "DebugPrint.h"

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

	bool HasRTC(CartridgeType type)
	{
		switch (type)
		{
		case CartridgeType::MBC3_TIMER_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY_2:
			return true;
		}

		return false;
	}

	bool IsMBC5(CartridgeType type)
	{
		switch (type)
		{
		case CartridgeType::MBC5:
		case CartridgeType::MBC5_RAM:
		case CartridgeType::MBC5_RAM_BATTERY:
		case CartridgeType::MBC5_RUMBLE:
		case CartridgeType::MBC5_RUMBLE_RAM:
		case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
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
		else if (IsMBC5(header.type))
		{
			return std::make_unique<MBC5>(cartridgePath);
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
	}

	void MBC::loadSRAM()
	{
		if (ramSizeBytes() == 0) return;

		const auto savPath = GetSaveFilePath(cartridgePath_);
		if (FileSystem::Exists(savPath))
		{
			loadSRAM_(savPath);
		}
	}

	void MBC::loadSRAM_(FilePathView saveFilePath)
	{
		BinaryReader savReader{ saveFilePath };
		savReader.read(sram_.data(), Min<size_t>(sram_.size(), savReader.size()));
	}

	void MBC::saveSRAM()
	{
		if (ramSizeBytes() == 0) return;

		const auto savPath = GetSaveFilePath(cartridgePath_);
		saveSRAM_(savPath);
	}

	void MBC::saveSRAM_(FilePathView saveFilePath)
	{
		BinaryWriter writer{ saveFilePath };
		writer.write(sram_.data(), ramSizeBytes());
	}

	CGBFlag MBC::cgbFlag() const
	{
		return cartridgeHeader_.cgbFlag;
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
		cartridgeHeader_.dump();
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

			if (not ramEnabled_)
			{
				return 0xff;
			}

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

			if (not ramEnabled_)
			{
				return 0xff;
			}

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
			// RAM and Timer Enable

			bool enabled = (value & 0xf) == 0xa;
			ramEnabled_ = enabled;
			rtc_.setEnable(enabled);
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
			// RAM Bank Number / RTC Register Select
			if (value <= 3)
			{
				MBC1::write(addr, value);

				rtc_.unselect();
			}
			else
			{
				// Map the RTC register
				rtc_.select(value);
			}
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_LatchClock))
		{
			rtc_.writeLatchClock(value);
		}
		else if (ADDRESS_IN_RANGE(addr, Address::SRAM))
		{
			if (rtc_.selected())
			{
				rtc_.writeRegister(value);
			}
			else
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
			// External RAM or RTC Register

			if (rtc_.selected())
			{
				return rtc_.readRegister();
			}
			else
			{
				if (not ramEnabled_)
				{
					return 0xff;
				}

				const uint16 offset = addr - Address::SRAM;
				return sram_[ramBank_ * 0x2000 + offset];
			}
		}

		return 0;
	}

	void MBC3::update(int cycles)
	{
		rtc_.update(cycles);
	}

	void MBC3::loadSRAM_(FilePathView saveFilePath)
	{
		BinaryReader savReader{ saveFilePath };

		// メインのセーブデータ: ramSizeBytes バイト読み込む
		if (savReader.size() >= ramSizeBytes())
		{
			savReader.read(sram_.data(), ramSizeBytes());
		}

		// RTC
		if (HasRTC(cartridgeHeader_.type) && savReader.size() >= ramSizeBytes() + sizeof(RTCSaveData))
		{
			RTCSaveData rtcSaveData;
			savReader.read(rtcSaveData);

			rtc_.loadSaveData(rtcSaveData);
		}
	}

	void MBC3::saveSRAM_(FilePathView saveFilePath)
	{
		BinaryWriter writer{ saveFilePath };

		// メインのセーブデータ: ramSizeBytes バイト書き込む
		writer.write(sram_.data(), ramSizeBytes());

		// RTC
		if (HasRTC(cartridgeHeader_.type))
		{
			const auto rtcSaveData = rtc_.getSaveData();
			writer.write(rtcSaveData);
		}
	}

	// ------------------------------------------------
	// MBC5
	// ------------------------------------------------

	void MBC5::write(uint16 addr, uint8 value)
	{
		if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMEnable))
		{
			// RAM and Timer Enable

			bool enabled = (value & 0xf) == 0xa;
			ramEnabled_ = enabled;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBankLow))
		{
			// The 8 least significant bits of the ROM bank number

			romBank_ &= 0x100;
			romBank_ |= value;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_ROMBankHigh))
		{
			// The 9th bit of the ROM bank number

			romBank_ &= 0xff;
			romBank_ |= (value & 1) << 8;
		}
		else if (ADDRESS_IN_RANGE(addr, Address::MBC_RAMBank))
		{
			// RAM Bank Number
			if (value <= 0xf)
			{
				MBC1::write(addr, value);
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

			const uint16 offset = addr - Address::SRAM;
			sram_[ramBank_ * 0x2000 + offset] = value;
		}
	}

	uint8 MBC5::read(uint16 addr) const
	{
		if (addr <= Address::ROMBank0_End)
		{
			// ROM Bank 0
			return rom_[addr];
		}
		else if (addr <= Address::SwitchableROMBank_End)
		{
			// ROM Bank 0-
			return MBC1::read(addr);
		}
		else if (addr <= Address::SRAM_End)
		{
			// External RAM

			if (not ramEnabled_)
			{
				return 0xff;
			}

			const uint16 offset = addr - Address::SRAM;
			return sram_[ramBank_ * 0x2000 + offset];
		}

		return 0;
	}
}
