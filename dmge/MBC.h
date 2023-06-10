#pragma once

#include "Cartridge.h"

namespace dmge
{
	class MBC
	{
	public:
		MBC(FilePath cartridgePath);

		virtual ~MBC() = default;

		virtual void write(uint16 addr, uint8 value) = 0;

		virtual uint8 read(uint16 addr) const = 0;

		static std::unique_ptr<MBC> LoadCartridge(FilePath cartridgePath);

		void loadSRAM();

		void saveSRAM();

		int romBank() const;

		int ramBank() const;

		int ramSizeBytes() const;

		// [DEBUG]
		void dumpCartridgeInfo();


	protected:
		String cartridgePath_;

		// カートリッジの内容
		Array<uint8> rom_;

		// External RAM (SRAM)
		std::array<uint8, 0x8000> sram_;

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

		virtual void write(uint16 addr, uint8 value) override;

		virtual uint8 read(uint16 addr) const override;
	};

	class MBC2 : public MBC1
	{
	public:
		using MBC1::MBC1;

		virtual void write(uint16 addr, uint8 value) override;

		virtual uint8 read(uint16 addr) const override;
	};

	class MBC3 : public MBC1
	{
	public:
		using MBC1::MBC1;

		virtual void write(uint16 addr, uint8 value) override;

		virtual uint8 read(uint16 addr) const override;
	};
}
