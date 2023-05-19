#pragma once

#include "Colors.h"

namespace dmge
{
	class Memory;

	// LCD Register (0xff40-0xff4b)

	class LCD
	{
	public:
		LCD(Memory* mem);

		// LCD Control (0xff40)
		bool isEnabled() const;
		uint16 windowTileMapAddress() const;
		bool isEnabledWindow() const;
		uint16 tileDataAddress() const;
		uint16 bgTileMapAddress() const;
		bool isEnabledTallSprite() const;
		bool isEnabledSprite() const;
		bool isEnabledBgAndWindow() const;

		// LCD Status (0xff41)
		uint8 stat() const;
		bool isEnabledLYCInterrupt() const;
		bool isEnabledOAMScanInterrupt() const;
		bool isEnabledVBlankInterrupt() const;
		bool isEnabledHBlankInterrupt() const;
		bool lycFlag() const;

		// SCY, SCX (0xff42-43)
		uint8 scy() const;
		uint8 scx() const;

		// LY (0xff44)
		uint8 ly() const;
		void ly(uint8 value);

		// LYC (0xff45)
		uint8 lyc() const;

		// BGP (0xff47)
		Array<Colors::Gray> bgp() const;

		// OBP (0xff48-49)
		std::array<Colors::Gray, 4 * 2> obp() const;
		//Array<Colors::Gray> obp0() const;
		//Array<Colors::Gray> obp1() const;

		// WY, WX (0xff4a-4b)
		uint8 wy() const;
		uint8 wx() const;

	private:
		Memory* mem_;
	};
}
