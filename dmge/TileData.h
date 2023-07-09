#pragma once

namespace dmge
{
	namespace TileData
	{
		// タイルデータの、上から row 行目のアドレスを得る
		uint16 GetAddress(uint16 baseAddr, uint8 tileId, uint8 row = 0, bool yFlip = false);

		// タイルデータの、左から dotNth 個目のドットの色番号を得る
		uint8 GetColor(uint16 tileData, int dotNth, bool xFlip = false);
	}


	class Memory;

	class TileDataTexture
	{
	public:
		TileDataTexture(Memory& mem);

		void update();

		RectF draw(const Vec2& pos) const;

		Size size() const;

	private:
		Memory& mem_;
		Image tileImage_;
		DynamicTexture texture_;
	};
}
