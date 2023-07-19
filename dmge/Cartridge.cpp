#include "Cartridge.h"
#include "Address.h"
#include "DebugPrint.h"

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include "lib/magic_enum/magic_enum.hpp"

namespace dmge
{
	CartridgeHeader::CartridgeHeader(FilePath cartridgePath)
	{
		BinaryReader reader{ cartridgePath };
		Array<uint8> header;
		header.resize(0x150);
		reader.read(header.data(), header.size());

		// Title
		title = header.slice(Address::Title, 15).map([](uint8 x) { return static_cast<char32_t>(x); }).join(U""_sv, U""_sv, U""_sv);

		// CGB flag
		cgbFlag = static_cast<CGBFlag>(header[Address::CGBFlag]);

		// Type
		type = static_cast<CartridgeType>(header[Address::CartridgeType]);

		// ROM size

		const uint8 romSize = header[Address::ROMSize];
		romSizeKB = 32 * (1 << romSize);

		romBankMax = romSizeKB / 16;

		// RAM size

		const uint8 ramSize = header[Address::RAMSize];

		switch (ramSize)
		{
			case 0: ramSizeKB = 0; break;
			case 1: ramSizeKB = 0; break;
			case 2: ramSizeKB = 8; break;
			case 3: ramSizeKB = 32; break;
			case 4: ramSizeKB = 128; break;
			case 5: ramSizeKB = 64; break;
		}
	}

	void CartridgeHeader::dump()
	{
		DebugPrint::Writeln(U"* Cartridge Info:");
		DebugPrint::Writeln(U"Title={}"_fmt(title));

		const auto cgbFlagStr = magic_enum::enum_name(cgbFlag);
		DebugPrint::Writeln(U"CGBFlag={}"_fmt(Unicode::WidenAscii(cgbFlagStr)));

		const auto typeStr = magic_enum::enum_name(type);
		DebugPrint::Writeln(U"Type={}"_fmt(Unicode::WidenAscii(typeStr)));

		DebugPrint::Writeln(U"RomSize={}KB"_fmt(romSizeKB));
		DebugPrint::Writeln(U"RamSize={}KB"_fmt(ramSizeKB));
	}

	bool IsValidCartridgePath(StringView path)
	{
		return FileSystem::Exists(path) && FileSystem::IsFile(path);

	}

	FilePath GetSaveFilePath(FilePathView cartridgePath)
	{
		return FileSystem::PathAppend(FileSystem::ParentPath(cartridgePath), FileSystem::BaseName(cartridgePath)) + U".sav";
	}
}
