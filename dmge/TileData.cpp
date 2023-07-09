#include "TileData.h"
#include "Memory.h"
#include "Address.h"

namespace dmge
{
	namespace TileData
	{
		uint16 GetAddress(uint16 baseAddr, uint8 tileId, uint8 row, bool yFlip)
		{
			const auto yShift = yFlip ? 2 * (7 - row) : 2 * row;
			return baseAddr
				+ (baseAddr == 0x8000 ? tileId : (int8)tileId) * 0x10
				+ yShift;
		}

		uint8 GetColor(uint16 tileData, int dotNth, bool xFlip)
		{
			const auto bitShift = xFlip ? dotNth : 7 - dotNth;
			return ((tileData >> bitShift) & 1) | (((tileData >> (bitShift + 8)) & 1) << 1);
		}
	}

	constexpr Size TileImageSize{ 8 * 16, 8 * 24 };

	TileDataTexture::TileDataTexture(Memory& mem)
		: mem_{ mem }, tileImage_{ TileImageSize }, texture_{ TileImageSize }
	{
	}

	void TileDataTexture::update()
	{
		for (uint16 addr = Address::TileData0; addr < Address::TileData2_End; addr += 0x10)
		{
			const Size tileTopLeftPos{
				((addr - Address::TileData0) / 2) % TileImageSize.x,
				((addr - Address::TileData0) / 2) / TileImageSize.x * 8
			};

			for (int y = 0; y < 8; ++y)
			{
				const uint16 tileData = mem_.read16(addr + y * 2);

				for (int x = 0, bit = 7; x < 8; ++x, --bit)
				{
					const uint8 colorID = (((tileData & 0xff) >> bit) & 1) | (((((tileData >> 8) & 0xff) >> bit) & 1) << 1);
					tileImage_[tileTopLeftPos.y + y][tileTopLeftPos.x + x] = Color{ colorID * (255u / 3), 255u };
				}
			}
		}

		texture_.fill(tileImage_);
	}

	RectF TileDataTexture::draw(const Vec2& pos) const
	{
		return texture_.draw(Arg::topLeft = pos);
	}

	Size TileDataTexture::size() const
	{
		return texture_.size();
	}
}
