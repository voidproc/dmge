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
}
