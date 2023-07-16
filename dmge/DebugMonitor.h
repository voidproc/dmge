#pragma once

#include "TileData.h"

namespace dmge
{
	class Memory;
	class CPU;
	class APU;
	class Interrupt;

	class DebugMonitor
	{
	public:
		// 表示サイズは 横90 * 縦40 [chars] くらい
		inline static constexpr Size ViewportSize{ 5 * 90, 10 * 40 };

		// 背景色
		inline static constexpr Color BgColor{ 32 };

		DebugMonitor(Memory* mem, CPU* cpu, APU* apu, Interrupt* interrupt);

		~DebugMonitor();

		void setCGBMode(bool value);

		void update();

		void updateGUI();

		void draw(const Point& pos) const;

		bool isVisibleTextbox() const;

	private:
		Memory* mem_;
		CPU* cpu_;
		APU* apu_;
		Interrupt* interrupt_;

		bool cgbMode_ = false;

		// メモリダンプ用

		uint16 dumpAddress_ = 0x0000;
		bool showDumpAddressTextbox_ = false;
		TextEditState textStateDumpAddress_;

		// タイルデータ表示用
		TileDataTexture tileDataTexture_;
		TileDataTexture tileDataTextureCGB_;
	};
}
