#pragma once

#include "Colors.h"
#include "Address.h"
#include "PPUMode.h"

namespace dmge
{
	class Memory;

	// LCDレジスタ（0xff40-0xff4b, 0xff68-0xff6c）の表示・制御

	class LCD
	{
	public:
		LCD(Memory& mem);

		// LCD Control (0xff40)
		uint8 lcdc() const;

		// LCDC.7 - LCD enable
		bool isEnabled() const;

		// LCDC.6 - Window tile map area
		// 0x9800 or 0x9C00
		uint16 windowTileMapAddress() const;

		// LCDC.5 - Window enable
		bool isEnabledWindow() const;

		// LCDC.4 - BG and Window tile data area
		// 0x8000 or 0x9000
		uint16 tileDataAddress() const;

		// LCDC.3 - BG tile map area
		// 0x9800 or 0x9C00
		uint16 bgTileMapAddress() const;

		// LCDC.2 - OBJ size
		uint8 spriteSize() const;

		// LCDC.1 - OBJ enable
		bool isEnabledSprite() const;

		// LCDC.0 - BG and Window enable/priority
		bool isEnabledBgAndWindow() const;

		// LCD Status (0xff41)
		uint8 stat() const;

		// STAT.6 - LYC=LY STAT Interrupt source
		bool isEnabledLYCInterrupt() const;

		// STAT.5 - Mode 2 OAM STAT Interrupt source
		bool isEnabledOAMScanInterrupt() const;

		// STAT.4 - Mode 1 VBlank STAT Interrupt source
		bool isEnabledVBlankInterrupt() const;

		// STAT.3 - Mode 0 HBlank STAT Interrupt source
		bool isEnabledHBlankInterrupt() const;

		// STAT.2 - LYC = LY Flag
		bool lycFlag() const;

		void setSTATLYCFlag(bool flag);

		void setSTATPPUMode(PPUMode mode);

		// SCY (0xff42)
		uint8 scy() const;

		// SCX (0xff43)
		uint8 scx() const;

		// LY (0xff44)
		uint8 ly() const;

		void ly(uint8 value);

		// LYC (0xff45)
		uint8 lyc() const;

		// BGP (0xff47)
		Colors::Gray bgp(int n) const;

		// OBP (0xff48-49)
		// palette: 0=OBP0, 1=OBP1
		Colors::Gray obp(int palette, int n) const;

		// WY (0xff4a)
		uint8 wy() const;

		// WX (0xff4b)
		uint8 wx() const;

		// BGPI
		uint8 bgPaletteIndex() const;

		// BGPD
		bool bgPaletteIndexAutoIncrement() const;

		// OBPI
		uint8 objPaletteIndex() const;

		// OBPD
		bool objPaletteIndexAutoIncrement() const;

		// OPRI
		uint8 opri() const;

		// (CGB) 実際の描画色
		const ColorF& bgPaletteColor(uint8 palette, uint8 color) const;

		// (CGB) 実際の描画色
		const ColorF& objPaletteColor(uint8 palette, uint8 color) const;

		// IOレジスタへの書き込み
		void writeRegister(uint16 addr, uint8 value);

		// IOレジスタからの読み込み
		uint8 readRegister(uint16 addr) const;

		// (SGB) PAL_TRN
		void transferSystemColorPalette();

	private:
		Memory& mem_;

		// LCD Registers

		uint8 lcdc_ = 0;
		uint8 stat_ = 0;
		uint8 scy_ = 0;
		uint8 scx_ = 0;
		uint8 ly_ = 0;
		uint8 lyc_ = 0;
		uint8 bgp_ = 0;
		uint8 obp0_ = 0;
		uint8 obp1_ = 0;
		uint8 wy_ = 0;
		uint8 wx_ = 0;
		uint8 opri_ = 0;

		// LCDC state
		uint8 spriteSize_ = 8;
		uint16 windowTileMapAddress_ = Address::TileMap0;
		uint16 bgTileMapAddress_ = Address::TileMap0;
		uint16 tileDataAddress_ = Address::TileData0;

		// (CGB) BG/OBJ Palette Index

		uint8 bgPaletteIndex_ = 0;
		bool bgPaletteIndexAutoIncr_ = false;
		uint8 objPaletteIndex_ = 0;
		bool objPaletteIndexAutoIncr_ = false;

		// (CGB) BG/OBJ Palette

		std::array<uint8, 64> bgPalette_{};
		std::array<uint8, 64> objPalette_{};

		// (CGB) 色番号から実際の色への変換テーブル

		std::array<std::array<ColorF, 4>, 8> displayBGColorPalette_{};
		std::array<std::array<ColorF, 4>, 8> displayOBJColorPalette_{};

		// (SGB) System Color Palette
		std::array<uint16, 512 * 4> sgbSystemColorPalette_{};
	};
}
