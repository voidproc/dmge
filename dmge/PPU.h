#pragma once

#include "Memory.h"
#include "PPUMode.h"
#include "Colors.h"

namespace dmge
{
	// OAMバッファ
	struct BufferedOAM
	{
		uint8 y;
		uint8 x;
		uint8 tile;

		// Flags
		uint8 priority;
		bool yFlip;
		bool xFlip;
		uint8 palette;
		uint8 cgbFlag;
	};

	class LCD;

	class PPU
	{
	public:
		PPU(Memory* mem, LCD* lcd);

		void run();

		void draw(const Point pos, int scale);

		PPUMode mode() const
		{
			return mode_;
		}

		bool modeChangedToOAMScan() const;
		bool modeChangedToHBlank() const;
		bool modeChangedToVBlank() const;

		int dot() const
		{
			return dot_;
		}

	private:
		Memory* mem_;
		LCD* lcd_;
		int dot_ = 0;
		uint8 prevLYC_ = 0;
		PPUMode mode_ = PPUMode::OAMScan;
		bool prevStatInt_ = false;

		// Pixel FIFO
		Grid<Color> canvas_;
		int fetcherX_ = 0;
		int canvasX_ = 0;
		bool toDrawWindow_ = false; //フレーム内でLY==WYとなった
		bool drawingWindow_ = false; //現在、ウィンドウをフェッチしている
		int windowLine_ = 0;
		Array<BufferedOAM> oamBuffer_;


		// 色番号から実際の色への変換テーブル
		const HashTable<Colors::Gray, Color> paletteColorMap_{
			{ Colors::Gray::White, Color{ 221, 255, 212 } },
			{ Colors::Gray::LightGray, Palette::Lightgreen },
			{ Colors::Gray::DrakGray, Color{ 29, 114, 61 } },
			{ Colors::Gray::Black, Color{ 0, 51, 0 } },
		};

		void updateLY_();
		void updateSTAT_();
		void updateMode_();
		void scanline_();
		void scanOAM_();
	};
}
