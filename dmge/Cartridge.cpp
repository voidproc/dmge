#include "Cartridge.h"
#include "Address.h"

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

		// Type
		type = static_cast<CartridgeType>(header[Address::CartridgeType]);

		// ROM Size

		const uint8 romSize = header[Address::ROMSize];
		romSizeKB = 32 * (1 << romSize);

		// RAM Size

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
}
