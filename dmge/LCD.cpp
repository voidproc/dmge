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

	bool LCD::isEnabled() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::LCDEnable) > 0;
	}

	uint16 LCD::windowTileMapAddress() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::WindowTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
	}

	bool LCD::isEnabledWindow() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::WindowEnable) > 0;
	}

	uint16 LCD::tileDataAddress() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::BGAndWindowTileDataArea) > 0 ? Address::TileData0 : Address::TileData2;
	}

	uint16 LCD::bgTileMapAddress() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::BGTileMapArea) > 0 ? Address::TileMap1 : Address::TileMap0;
	}

	bool LCD::isEnabledTallSprite() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::OBJSize) > 0;
	}

	bool LCD::isEnabledSprite() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::OBJEnable) > 0;
	}

	bool LCD::isEnabledBgAndWindow() const
	{
		const uint8 lcdc = mem_->read(Address::LCDC);
		return (lcdc & BitMask::LCDC::BGAndWindowEnable) > 0;
	}

	uint8 LCD::stat() const
	{
		return mem_->read(Address::STAT);
	}

	bool LCD::isEnabledLYCInterrupt() const
	{
		const uint8 stat = mem_->read(Address::STAT);
		return (stat & BitMask::STAT::LYCInterruptEnable) > 0;
	}

	bool LCD::isEnabledOAMScanInterrupt() const
	{
		const uint8 stat = mem_->read(Address::STAT);
		return (stat & BitMask::STAT::OAMInterruptEnable) > 0;
	}

	bool LCD::isEnabledVBlankInterrupt() const
	{
		const uint8 stat = mem_->read(Address::STAT);
		return (stat & BitMask::STAT::VBlankInterruptEnable) > 0;
	}

	bool LCD::isEnabledHBlankInterrupt() const
	{
		const uint8 stat = mem_->read(Address::STAT);
		return (stat & BitMask::STAT::HBlankInterruptEnable) > 0;
	}

	bool LCD::lycFlag() const
	{
		const uint8 stat = mem_->read(Address::STAT);
		return (stat & BitMask::STAT::LYCFlag) > 0;
	}

	uint8 LCD::scy() const
	{
		return mem_->read(Address::SCY);
	}

	uint8 LCD::scx() const
	{
		return mem_->read(Address::SCX);
	}

	uint8 LCD::ly() const
	{
		return mem_->read(Address::LY);
	}

	void LCD::ly(uint8 value)
	{
		mem_->write(Address::LY, value);
	}

	uint8 LCD::lyc() const
	{
		return mem_->read(Address::LYC);
	}

	Colors::Gray LCD::bgp(int n)
	{
		const uint8 paletteData = mem_->read(Address::BGP);
		return static_cast<Colors::Gray>((paletteData >> (n * 2)) & 0b11);
	}

	Colors::Gray LCD::obp(int palette, int n)
	{
		const uint8 paletteData = mem_->read(palette == 0 ? Address::OBP0 : Address::OBP1);
		return static_cast<Colors::Gray>((paletteData >> (n * 2)) & 0b11);
	}

	uint8 LCD::wy() const
	{
		return mem_->read(Address::WY);
	}

	uint8 LCD::wx() const
	{
		return mem_->read(Address::WX);
	}
}
