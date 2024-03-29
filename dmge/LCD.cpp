﻿#include "stdafx.h"
#include "LCD.h"
#include "Memory.h"
#include "BitMask/LCDC.h"
#include "BitMask/STAT.h"

namespace dmge
{
	namespace
	{
		ColorF ConvertColorFrom555(uint16 color)
		{
			return ColorF{
				((color >> 0) & 0x1f) * 1.0 / 0x1f,
				((color >> 5) & 0x1f) * 1.0 / 0x1f,
				((color >> 10) & 0x1f) * 1.0 / 0x1f
			};
		}
	}

	LCD::LCD(Memory& mem)
		: mem_{ mem }
	{
		for (int i = 0; i < sgbPaletteColors_.size(); ++i)
		{
			sgbPaletteColors_[i][0] = ColorF{ U"#e8e8e8" };
			sgbPaletteColors_[i][1] = ColorF{ U"#a0a0a0" };
			sgbPaletteColors_[i][2] = ColorF{ U"#585858" };
			sgbPaletteColors_[i][3] = ColorF{ U"#101010" };
		}
	}

	uint8 LCD::lcdc() const
	{
		return lcdc_;
	}

	bool LCD::isEnabled() const
	{
		return (lcdc_ & BitMask::LCDC::LCDEnable) > 0;
	}

	uint16 LCD::windowTileMapAddress() const
	{
		return windowTileMapAddress_;
	}

	bool LCD::isEnabledWindow() const
	{
		return (lcdc_ & BitMask::LCDC::WindowEnable) > 0;
	}

	uint16 LCD::tileDataAddress() const
	{
		return tileDataAddress_;
	}

	uint16 LCD::tileDataAddressTop() const
	{
		return tileDataAddressTop_;
	}

	uint16 LCD::bgTileMapAddress() const
	{
		return bgTileMapAddress_;
	}

	uint8 LCD::spriteSize() const
	{
		return spriteSize_;
	}

	bool LCD::isEnabledSprite() const
	{
		return (lcdc_ & BitMask::LCDC::OBJEnable) > 0;
	}

	bool LCD::isEnabledBgAndWindow() const
	{
		return (lcdc_ & BitMask::LCDC::BGAndWindowEnable) > 0;
	}

	uint8 LCD::stat() const
	{
		return stat_;
	}

	bool LCD::isEnabledLYCInterrupt() const
	{
		return (stat_ & BitMask::STAT::LYCInterruptEnable) > 0;
	}

	bool LCD::isEnabledOAMScanInterrupt() const
	{
		return (stat_ & BitMask::STAT::OAMInterruptEnable) > 0;
	}

	bool LCD::isEnabledVBlankInterrupt() const
	{
		return (stat_ & BitMask::STAT::VBlankInterruptEnable) > 0;
	}

	bool LCD::isEnabledHBlankInterrupt() const
	{
		return (stat_ & BitMask::STAT::HBlankInterruptEnable) > 0;
	}

	bool LCD::lycFlag() const
	{
		return (stat_ & BitMask::STAT::LYCFlag) > 0;
	}

	void LCD::setSTATLYCFlag(bool flag)
	{
		stat_ = 0x80 | (stat_ & ~4) | (flag ? 4 : 0);
	}

	void LCD::setSTATPPUMode(PPUMode mode)
	{
		// LCDがOFFの場合は mode=0 をセットする
		stat_ = 0x80 | (stat_ & ~3) | (isEnabled() ? FromEnum(mode) : 0);
	}

	uint8 LCD::scy() const
	{
		return scy_;
	}

	uint8 LCD::scx() const
	{
		return scx_;
	}

	uint8 LCD::ly() const
	{
		return ly_;
	}

	void LCD::ly(uint8 value)
	{
		ly_ = value;
	}

	uint8 LCD::lyc() const
	{
		return lyc_;
	}

	Colors::Gray LCD::bgp(int n) const
	{
		return ToEnum<Colors::Gray>((bgp_ >> (n * 2)) & 0b11);
	}

	Colors::Gray LCD::obp(int palette, int n) const
	{
		const uint8& paletteData = palette == 0 ? obp0_ : obp1_;
		return ToEnum<Colors::Gray>((paletteData >> (n * 2)) & 0b11);
	}

	uint8 LCD::wy() const
	{
		return wy_;
	}

	uint8 LCD::wx() const
	{
		return wx_;
	}

	uint8 LCD::bgPaletteIndex() const
	{
		return bgPaletteIndex_;
	}

	bool LCD::bgPaletteIndexAutoIncrement() const
	{
		return bgPaletteIndexAutoIncr_;
	}

	uint8 LCD::objPaletteIndex() const
	{
		return objPaletteIndex_;
	}

	bool LCD::objPaletteIndexAutoIncrement() const
	{
		return objPaletteIndexAutoIncr_;
	}

	uint8 LCD::opri() const
	{
		return opri_;
	}

	const ColorF& LCD::bgPaletteColor(uint8 palette, uint8 color) const
	{
		return bgPaletteColors_[palette][color];
	}

	const ColorF& LCD::objPaletteColor(uint8 palette, uint8 color) const
	{
		return objPaletteColors_[palette][color];
	}

	void LCD::writeRegister(uint16 addr, uint8 value)
	{
		if (addr == Address::LCDC)
		{
			lcdc_ = value;

			spriteSize_ = (value & BitMask::LCDC::OBJSize) > 0 ? 16 : 8;
			windowTileMapAddress_ = (value & BitMask::LCDC::WindowTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
			bgTileMapAddress_ = (value & BitMask::LCDC::BGTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
			tileDataAddress_ = (value & BitMask::LCDC::BGAndWindowTileDataArea) > 0 ? Address::TileData0 : Address::TileData2;
			tileDataAddressTop_ = (value & BitMask::LCDC::BGAndWindowTileDataArea) > 0 ? Address::TileData0 : Address::TileData1;
		}
		else if (addr == Address::STAT)
		{
			stat_ = value;
		}
		else if (addr == Address::SCY)
		{
			scy_ = value;
		}
		else if (addr == Address::SCX)
		{
			scx_ = value;
		}
		else if (addr == Address::LY)
		{
			ly_ = value;
		}
		else if (addr == Address::LYC)
		{
			lyc_ = value;
		}
		else if (addr == Address::BGP)
		{
			bgp_ = value;
		}
		else if (addr == Address::OBP0)
		{
			obp0_ = value;
		}
		else if (addr == Address::OBP1)
		{
			obp1_ = value;
		}
		else if (addr == Address::WY)
		{
			wy_ = value;
		}
		else if (addr == Address::WX)
		{
			wx_ = value;
		}
		else if (addr == Address::BCPS)
		{
			bgPaletteIndex_ = value & 0x3f;
			bgPaletteIndexAutoIncr_ = value >> 7;
		}
		else if (addr == Address::BCPD)
		{
			bgPalette_[bgPaletteIndex_] = value;

			const int pal = bgPaletteIndex_ / 8;
			const int nColor = (bgPaletteIndex_ / 2) % 4;
			const uint16 color = bgPalette_[pal * 8 + nColor * 2] | (bgPalette_[pal * 8 + nColor * 2 + 1] << 8);

			bgPaletteColors_[pal][nColor] = ConvertColorFrom555(color);

			if (bgPaletteIndexAutoIncr_)
			{
				bgPaletteIndex_ = (bgPaletteIndex_ + 1) % 64;
			}
		}
		else if (addr == Address::OCPS)
		{
			objPaletteIndex_ = value & 0x3f;
			objPaletteIndexAutoIncr_ = value >> 7;
		}
		else if (addr == Address::OCPD)
		{
			objPalette_[objPaletteIndex_] = value;

			const int pal = objPaletteIndex_ / 8;
			const int nColor = (objPaletteIndex_ / 2) % 4;
			const uint16 color = objPalette_[pal * 8 + nColor * 2] | (objPalette_[pal * 8 + nColor * 2 + 1] << 8);

			objPaletteColors_[pal][nColor] = ConvertColorFrom555(color);

			if (objPaletteIndexAutoIncr_)
			{
				objPaletteIndex_ = (objPaletteIndex_ + 1) % 64;
			}
		}
		else if (addr == Address::OPRI)
		{
			opri_ = value;
		}
	}

	uint8 LCD::readRegister(uint16 addr) const
	{
		if (addr == Address::LCDC)
		{
			return lcdc_;
		}
		else if (addr == Address::STAT)
		{
			return stat_;
		}
		else if (addr == Address::SCY)
		{
			return scy_;
		}
		else if (addr == Address::SCX)
		{
			return scx_;
		}
		else if (addr == Address::LY)
		{
			return ly_;
		}
		else if (addr == Address::LYC)
		{
			return lyc_;
		}
		else if (addr == Address::BGP)
		{
			return bgp_;
		}
		else if (addr == Address::OBP0)
		{
			return obp0_;
		}
		else if (addr == Address::OBP1)
		{
			return obp1_;
		}
		else if (addr == Address::WY)
		{
			return wy_;
		}
		else if (addr == Address::WX)
		{
			return wx_;
		}
		else if (addr == Address::BCPS)
		{
			return (bgPaletteIndex_ & 0x3f) | ((uint8)bgPaletteIndexAutoIncr_ << 7);
		}
		else if (addr == Address::BCPD)
		{
			return bgPalette_[bgPaletteIndex_];
		}
		else if (addr == Address::OCPS)
		{
			return (objPaletteIndex_ & 0x3f) | ((uint8)objPaletteIndexAutoIncr_ << 7);
		}
		else if (addr == Address::OCPD)
		{
			return objPalette_[objPaletteIndex_];
		}
		else if (addr == Address::OPRI)
		{
			return opri_;
		}

		return 0;
	}

	void LCD::transferSystemColorPalette()
	{
		for (uint16 addrOffset = 0, index = 0; addrOffset < 0x1000; addrOffset += 2, ++index)
		{
			const uint16 addr = tileDataAddressTop() + addrOffset;
			sgbSystemColorPaletteMemory_[index] = mem_.read16(addr);
		}
	}

	void LCD::setSGBPalette(uint16 palette0, uint16 palette1, uint16 palette2, uint16 palette3)
	{
		for (int color = 0; color < 4; ++color)
		{
			sgbPaletteColors_[0][color] = ConvertColorFrom555(sgbSystemColorPaletteMemory_[palette0 * 4 + color]);
			sgbPaletteColors_[1][color] = ConvertColorFrom555(sgbSystemColorPaletteMemory_[palette1 * 4 + color]);
			sgbPaletteColors_[2][color] = ConvertColorFrom555(sgbSystemColorPaletteMemory_[palette2 * 4 + color]);
			sgbPaletteColors_[3][color] = ConvertColorFrom555(sgbSystemColorPaletteMemory_[palette3 * 4 + color]);
		}
	}

	void LCD::setSGBPalette(int palette, int colorId, uint16 color)
	{
		sgbPaletteColors_[palette][colorId] = ConvertColorFrom555(color);
	}

	const ColorF& LCD::sgbPaletteColor(uint8 palette, uint8 color) const
	{
		return sgbPaletteColors_[palette][color];
	}

	void LCD::setSGBPaletteColors(uint8 paletteIndex, const std::array<ColorF, 4>& paletteColors)
	{
		std::copy(paletteColors.cbegin(), paletteColors.cend(), sgbPaletteColors_[paletteIndex].begin());
	}

	//ColorF LCD::sgbSystemColorPaletteMemoryData(int palette, int color) const
	//{
	//	return ConvertColorFrom555(sgbSystemColorPaletteMemory_[palette * 4 + color]);
	//}
}
