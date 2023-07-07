#pragma once

namespace dmge
{
	// Object Attribute Memory (OAM / OBJ / Sprite)
	struct OAM
	{
		// OAMTable内のアドレス
		uint16 address;

		// OAMのY座標
		// 実際の描画位置は-16される
		uint8 y;

		// OAMのX座標
		// 実際の描画位置は-8される
		uint8 x;

		// タイルID
		// 0x8000からの符号なしオフセット
		uint8 tile;

		// BG and Window over OBJ
		uint8 priority;

		// 垂直反転
		bool yFlip;

		// 水平反転
		bool xFlip;

		// (DMG) パレット（0=OBP0, 1=OBP1）
		uint8 palette;

		// (CGB) Tile VRAM-Bank
		uint8 bank;

		// (CGB) Palette number
		uint8 obp;
	};
}
