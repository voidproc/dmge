#pragma once

#include "TileData.h"
#include "TextboxOverlay.h"

namespace dmge
{
	class Memory;
	class CPU;
	class APU;
	class Interrupt;

	class DebugMonitor
	{
	public:
		// 表示サイズは 横 90 [chars]、縦 144*3 [px] くらい
		inline static constexpr Size ViewportSize{ 5 * 90, 144 * 3 };

		// 背景色
		inline static constexpr Color BgColor{ 32 };

		DebugMonitor(Memory* mem, CPU* cpu, APU* apu, Interrupt* interrupt);

		~DebugMonitor();

		void update();

		//void updateGUI();

		void draw(const Point& pos) const;

		bool isVisibleTextbox() const;

	private:
		Memory* mem_;
		CPU* cpu_;
		APU* apu_;
		Interrupt* interrupt_;

		// メモリダンプ用

		GUI::TextboxOverlay textbox_{ U"0000", U"Address:" };
		uint16 dumpAddress_ = 0x0000;
		Stopwatch timerTextboxHidden_{};

		// タイルデータ表示用

		TileDataTexture tileDataTexture_;
		TileDataTexture tileDataTextureCGB_;
	};
}
