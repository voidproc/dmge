#pragma once

#include "Cartridge.h"
#include "RTC.h"

namespace dmge
{
	class MBC
	{
	public:
		MBC(FilePath cartridgePath);

		virtual ~MBC() = default;

		virtual void write(uint16 addr, uint8 value) = 0;

		virtual uint8 read(uint16 addr) const = 0;

		virtual void update(int cycles) {}

		static std::unique_ptr<MBC> LoadCartridge(FilePath cartridgePath);

		void loadSRAM();

		void saveSRAM();

		CGBFlag cgbFlag() const;

		int romBank() const;

		int ramBank() const;

		int ramSizeBytes() const;

		// [DEBUG]
		void dumpCartridgeInfo();

		void enableBootROM(FilePathView bootROMPath);

		void disableBootROM();

	private:
		virtual void loadSRAM_(FilePathView saveFilePath);
		virtual void saveSRAM_(FilePathView saveFilePath);

	protected:
		String cartridgePath_;

		// カートリッジの内容
		Array<uint8> rom_;

		// External RAM (SRAM)
		std::array<uint8, 0x20000> sram_;

		// BootROM
		Array<uint8> boot_;

		// カートリッジのヘッダ情報
		CartridgeHeader cartridgeHeader_;

		// ROM / RAM バンク
		int romBank_ = 1;
		int ramBank_ = 0;
		int ramEnabled_ = false;
		int bankingMode_ = 0;

		void loadCartridge_(FilePath cartridgePath);
	};

	class NoMBC : public MBC
	{
	public:
		using MBC::MBC;

		virtual void write(uint16 addr, uint8 value) override;

		virtual uint8 read(uint16 addr) const override;
	};

	class MBC1 : public MBC
	{
	public:
		using MBC::MBC;

		void write(uint16 addr, uint8 value) override;

		uint8 read(uint16 addr) const override;
	};

	class MBC2 : public MBC1
	{
	public:
		using MBC1::MBC1;

		void write(uint16 addr, uint8 value) override;

		uint8 read(uint16 addr) const override;
	};

	class MBC3 : public MBC1
	{
	public:
		using MBC1::MBC1;

		void write(uint16 addr, uint8 value) override;

		uint8 read(uint16 addr) const override;

		void update(int cycles) override;

	private:
		RTC rtc_;

		virtual void loadSRAM_(FilePathView saveFilePath) override;
		virtual void saveSRAM_(FilePathView saveFilePath) override;
	};

	class MBC5 : public MBC1
	{
	public:
		using MBC1::MBC1;

		void write(uint16 addr, uint8 value) override;

		uint8 read(uint16 addr) const override;

	};

	class HuC1 : public MBC1
	{
	public:
		using MBC1::MBC1;

		void write(uint16 addr, uint8 value) override;

		uint8 read(uint16 addr) const override;

	private:
		bool ir_ = false;
	};
}
