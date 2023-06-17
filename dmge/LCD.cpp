#include "LCD.h"
#include "Memory.h"
#include "Address.h"
#include "BitMask/LCDC.h"
#include "BitMask/STAT.h"

namespace dmge
{
	LCD::LCD(Memory* mem)
		: mem_{ mem }
	{
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
		return (lcdc_ & BitMask::LCDC::WindowTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
	}

	bool LCD::isEnabledWindow() const
	{
		return (lcdc_ & BitMask::LCDC::WindowEnable) > 0;
	}

	uint16 LCD::tileDataAddress() const
	{
		return (lcdc_ & BitMask::LCDC::BGAndWindowTileDataArea) > 0 ? Address::TileData0 : Address::TileData2;
	}

	uint16 LCD::bgTileMapAddress() const
	{
		return (lcdc_ & BitMask::LCDC::BGTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
	}

	bool LCD::isEnabledTallSprite() const
	{
		return (lcdc_ & BitMask::LCDC::OBJSize) > 0;
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
		mem_->writeDirect(Address::LY, value);
	}

	uint8 LCD::lyc() const
	{
		return lyc_;
	}

	Colors::Gray LCD::bgp(int n)
	{
		return static_cast<Colors::Gray>((bgp_ >> (n * 2)) & 0b11);
	}

	Colors::Gray LCD::obp(int palette, int n)
	{
		const uint8& paletteData = palette == 0 ? obp0_ : obp1_;
		return static_cast<Colors::Gray>((paletteData >> (n * 2)) & 0b11);
	}

	uint8 LCD::wy() const
	{
		return wy_;
	}

	uint8 LCD::wx() const
	{
		return wx_;
	}

	void LCD::writeRegister(uint16 addr, uint8 value)
	{
		if (addr == Address::LCDC)
		{
			lcdc_ = value;
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
	}
}
