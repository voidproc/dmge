#include "PPU.h"
#include "Memory.h"
#include "TileData.h"
#include "BitMask/InterruptFlag.h"

namespace dmge
{
	constexpr Size LCDSize{ 160, 144 };

	// OAMバッファ
	struct BufferedOAM
	{
		// OAMのY座標
		// 実際の描画位置は-16される
		uint8 y;

		// OAMのX座標
		// 実際の描画位置は-8される
		uint8 x;

		// タイルID
		// 0x8000からの符号なしオフセット
		uint8 tile;


		// Flags

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

	// (CGB) 背景マップ属性
	union TileMapAttribute
	{
		struct
		{
			uint8 palette : 3;
			uint8 bank : 1;
			uint8 unused1 : 1;
			bool xFlip : 1;
			bool yFlip : 1;
			uint8 priority : 1;
		} attr;
		uint8 value;
	};

	PPU::PPU(Memory* mem)
		:
		mem_{ mem },
		lcd_{ mem },
		canvas_{ LCDSize.x + 8, LCDSize.y },
		texture_{ canvas_.size() }
	{
		dot_ = 70224 - 52 + 4;
		canvas_.fill(Palette::White);
		oamBuffer_.reserve(10);

		for (int pal = 0; pal < 8; pal++)
		{
			for (int i = 0; i < 4; i++)
			{
				displayBGColorPalette_[pal][i].set(0, 0, 0);
				displayOBJColorPalette_[pal][i].set(0, 0, 0);
			}
		}
	}

	PPU::~PPU()
	{
	}

	void PPU::setCGBMode(bool value)
	{
		cgbMode_ = value;
	}

	void PPU::run()
	{
		// 1ドット進める
		dot_ = (dot_ + 1) % 70224;
		updateLY_();

		// PPU Modeを更新
		updateMode_();

		// LCDの状態を更新
		updateSTAT_();


		// スキャンラインの更新
		if (mode_ == PPUMode::OAMScan && (dot_ % 456) == 79)
		{
			oamBuffer_.clear();
			scanOAM_();
		}

		if (mode_ == PPUMode::Drawing)
		{
			if (fetcherX_ < LCDSize.x && canvasX_ < LCDSize.x)
			{
				scanline_();
			}
		}

		if (modeChangedToHBlank())
		{
			// 右端の残りのドットを描画
			while (canvasX_ < LCDSize.x)
			{
				scanline_();
			}

			fetcherX_ = 0;
			canvasX_ = 0;
			if (drawingWindow_) windowLine_++;
			drawingWindow_ = false;
		}

		if (modeChangedToVBlank())
		{
			toDrawWindow_ = false;
			drawingWindow_ = false;
			windowLine_ = 0;
		}

		// 割り込み

		uint8 intFlag = mem_->read(Address::IF);

		// VBlank割り込み要求

		if (modeChangedToVBlank())
		{
			intFlag |= BitMask::InterruptFlag::VBlank;
		}

		// STAT割り込み要求

		bool statInt = false;
		if (lcd_.isEnabledLYCInterrupt() && lcd_.lycFlag())
		{
			statInt = true;
		}
		if (lcd_.isEnabledOAMScanInterrupt() && modeChangedToOAMScan())
		{
			statInt = true;
		}
		if (lcd_.isEnabledHBlankInterrupt() && modeChangedToHBlank())
		{
			statInt = true;
		}
		if (lcd_.isEnabledVBlankInterrupt() && modeChangedToVBlank())
		{
			statInt = true;
		}

		if (statInt && !prevStatInt_)
		{
			intFlag |= BitMask::InterruptFlag::STAT;
		}

		prevStatInt_ = statInt;

		mem_->write(Address::IF, intFlag);

	}

	void PPU::draw(const Point& pos, int scale)
	{
		if (not lcd_.isEnabled()) return;

		const ScopedRenderStates2D renderState{ SamplerState::ClampNearest };

		const Transformer2D transformer{ Mat3x2::Scale(scale).translated(pos) };

		texture_.fill(canvas_);
		texture_.draw();
	}

	PPUMode PPU::mode() const
	{
		return mode_;
	}

	bool PPU::modeChangedToOAMScan() const
	{
		return (dot_ < LCDSize.y * 456) && ((dot_ % 456) == 0);
	}

	bool PPU::modeChangedToHBlank() const
	{
		return (dot_ < LCDSize.y * 456) && ((dot_ % 456) == 370);
	}

	bool PPU::modeChangedToVBlank() const
	{
		return dot_ == LCDSize.y * 456;
	}

	int PPU::dot() const
	{
		return dot_;
	}

	void PPU::setDisplayColorPalette(const std::array<Color, 4>& palette)
	{
		std::copy(palette.cbegin(), palette.cend(), displayColorPalette_.begin());
	}

	void PPU::setBGPaletteIndex(uint8 index, bool autoIncrement)
	{
		bgPaletteIndex_ = index;
		bgPaletteIndexAutoIncr_ = autoIncrement;
	}

	void PPU::setBGPaletteData(uint8 value)
	{
		bgPalette_[bgPaletteIndex_] = value;

		const int pal = bgPaletteIndex_ / 8;
		const int nColor = (bgPaletteIndex_ / 2) % 4;
		const uint16 color = bgPalette_[pal * 8 + nColor * 2] | (bgPalette_[pal * 8 + nColor * 2 + 1] << 8);

		displayBGColorPalette_[pal][nColor].set(
			((color >> 0) & 0x1f) * 1.0 / 0x1f,
			((color >> 5) & 0x1f) * 1.0 / 0x1f,
			((color >> 10) & 0x1f) * 1.0 / 0x1f);

		if (bgPaletteIndexAutoIncr_)
		{
			bgPaletteIndex_ = (bgPaletteIndex_ + 1) % 64;
			mem_->writeDirect(Address::BCPS, ((uint8)bgPaletteIndexAutoIncr_ << 7) | bgPaletteIndex_);
		}
	}

	void PPU::setOBJPaletteIndex(uint8 index, bool autoIncrement)
	{
		objPaletteIndex_ = index;
		objPaletteIndexAutoIncr_ = autoIncrement;
	}

	void PPU::setOBJPaletteData(uint8 value)
	{
		objPalette_[objPaletteIndex_] = value;

		const int pal = objPaletteIndex_ / 8;
		const int nColor = (objPaletteIndex_ / 2) % 4;
		const uint16 color = objPalette_[pal * 8 + nColor * 2] | (objPalette_[pal * 8 + nColor * 2 + 1] << 8);

		displayOBJColorPalette_[pal][nColor].set(
			((color >> 0) & 0x1f) * 1.0 / 0x1f,
			((color >> 5) & 0x1f) * 1.0 / 0x1f,
			((color >> 10) & 0x1f) * 1.0 / 0x1f);

		if (objPaletteIndexAutoIncr_)
		{
			objPaletteIndex_ = (objPaletteIndex_ + 1) % 64;
			mem_->writeDirect(Address::OCPS, ((uint8)objPaletteIndexAutoIncr_ << 7) | objPaletteIndex_);
		}
	}

	void PPU::drawCGBPalette()
	{
		const Point pos{ 0, 0 };

		for (auto pal : step(8))
		{
			for (auto col : step(4))
			{
				Rect{ pos + Point{ col * 24, pal * 24 }, Size{ 16, 16 } }.draw(displayBGColorPalette_[pal][col]);

				Rect{ pos + Point{ col * 24 + 24*6, pal * 24 }, Size{ 16, 16 } }.draw(displayOBJColorPalette_[pal][col]);
			}
		}
	}

	void PPU::writeRegister(uint16 addr, uint8 value)
	{
		lcd_.writeRegister(addr, value);
	}

	void PPU::updateLY_()
	{
		if (not lcd_.isEnabled())
		{
			dot_ = 0;
		}

		uint8 ly = static_cast<uint8>(dot_ / 456);
		lcd_.ly(ly);
	}

	void PPU::updateMode_()
	{
		// Set PPU Mode

		if (lcd_.ly() >= LCDSize.y)
		{
			mode_ = PPUMode::VBlank;
		}
		else
		{
			if (const int x = dot_ % 456; x < 80)
			{
				mode_ = PPUMode::OAMScan;
			}
			else if (x < 369)
			{
				mode_ = PPUMode::Drawing;
			}
			else
			{
				mode_ = PPUMode::HBlank;
			}
		}

		// Set STAT.1-0 (PPU Mode)
		// LCDがOFFの場合は mode=0 をセットする

		const uint8 stat = lcd_.stat();
		mem_->write(Address::STAT, 0x80 | (stat & ~3) | (lcd_.isEnabled() ? (uint8)mode_ : 0));
	}

	void PPU::updateSTAT_()
	{
		// Set STAT.2 (Coincidence Flag)
		// LYが更新されるかLYCが設定されるときにSTAT.2をセットする

		const bool changedLy = dot_ % 456 == 0;
		const uint8 lyc = lcd_.lyc();

		if (changedLy || lyc != prevLYC_)
		{
			const uint8 ly = lcd_.ly();
			const uint8 stat = lcd_.stat();
			mem_->write(Address::STAT, 0x80 | (stat & ~4) | (ly == lyc ? 4 : 0));
		}

		prevLYC_ = lyc;
	}

	void PPU::scanOAM_()
	{
		const uint8 ly = lcd_.ly();
		const int spriteSize = lcd_.isEnabledTallSprite() ? 16 : 8;

		// X座標が同じOAMをバッファに追加しないよう、
		// バッファに追加したOAMのX座標を記録しておく
		Array<uint8> xList;

		// OAMメモリを走査して、描画対象のOAMをバッファに格納していく
		for (uint16 addr = 0xfe00; addr <= 0xfe9f; addr += 4)
		{
			// 1ライン10個まで
			if (oamBuffer_.size() >= 10) break;

			BufferedOAM oam{};
			oam.y = mem_->read(addr);

			// Y座標が描画範囲にあるか？
			if (ly + 16 < oam.y) continue;
			if (ly + 16 >= oam.y + spriteSize) continue;

			oam.x = mem_->read(addr + 1);

			// X座標が描画範囲にあるか？
			if (oam.x == 0) continue;

			// 同じX座標のOAMを複数描画しない
			if (xList.contains(oam.x)) continue;

			xList << oam.x;

			oam.tile = mem_->read(addr + 2);

			const uint8 flags = mem_->read(addr + 3);
			oam.priority = (flags >> 7) & 1;
			oam.yFlip = (flags >> 6) & 1;
			oam.xFlip = (flags >> 5) & 1;
			oam.palette = (flags >> 4) & 1;
			oam.bank = (flags >> 3) & 1;
			oam.obp = flags & 0b111;

			// 8x16サイズのスプライトの場合、上半分のタイルIDはLSB=0、下半分はLSB=1となる
			// ただし yFlip==true の場合は反対になる
			if (spriteSize == 16)
			{
				if ((not oam.yFlip && (ly + 16 < oam.y + 8)) ||
					(oam.yFlip && (ly + 16 >= oam.y + 8)))
				{
					oam.tile &= 0xfe;
				}
				else
				{
					oam.tile |= 1;
				}
			}

			oamBuffer_.push_back(oam);
		}
	}

	void PPU::scanline_()
	{
		// DMG: BGとWindow無効
		// CGB: BGとWindowの優先度無効
		const bool lcdc0 = lcd_.isEnabledBgAndWindow();

		// (DMG) BGとWindowが無効なので、ピクセルを1つ進めて終了
		if (not cgbMode_ && not lcdc0)
		{
			canvas_[lcd_.ly()][canvasX_] = displayColorPalette_[0];

			fetcherX_++;
			canvasX_++;
			return;
		}

		// 現在の状態
		const uint8 ly = lcd_.ly();
		const uint8 scy = lcd_.scy();
		const uint8 scx = lcd_.scx();
		const uint8 wy = lcd_.wy();
		const uint8 wx = lcd_.wx();
		const bool enabledWindow = lcd_.isEnabledWindow();
		const uint8 opri = mem_->read(Address::OPRI) & 1;

		// ※スクロール処理
		// BG描画中（ウィンドウ描画中でないとき）、SCXの端数分を読み飛ばす
		if (not drawingWindow_ && (scx % 8) > 0 && fetcherX_ < (scx % 8))
		{
			fetcherX_++;
			return;
		}

		// このフレームでウィンドウに到達したかどうかを toDrawWindow_ に記録する
		if (ly == wy)
		{
			toDrawWindow_ = true;
		}

		// このスキャンラインでウィンドウのフェッチが開始したら
		// ピクセルフェッチャーのX座標をリセットする
		// ※スプライトの描画には、リセットされない canvasX_ を使用
		if (not drawingWindow_ && enabledWindow && toDrawWindow_ && (fetcherX_ >= wx - 7))
		{
			drawingWindow_ = true;
			fetcherX_ = 0;
		}

		// ピクセルフェッチャーのいるX座標
		const uint8 x = fetcherX_;


		// タイルマップの中の、描画対象のタイルのアドレスを得る
		uint16 tileAddr;

		if (drawingWindow_)
		{
			const uint16 tileMapAddrBase = lcd_.windowTileMapAddress();
			const uint16 addrOffsetX = x / 8;
			const uint16 addrOffsetY = 32 * (windowLine_ / 8);
			tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		}
		else
		{
			const uint16 tileMapAddrBase = lcd_.bgTileMapAddress();
			const uint16 addrOffsetX = ((x / 8) + (scx / 8)) & 0x1f;
			const uint16 addrOffsetY = 32 * (((ly + scy) & 0xff) / 8);
			tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		}

		// (CGB) 背景マップ属性を取得（VRAM Bank1）

		TileMapAttribute tileMapAttr{};

		if (cgbMode_)
		{
			tileMapAttr.value = mem_->readVRAMBank(tileAddr, 1);
		}

		// タイルデータのアドレスを得る
		const uint8 tileId = mem_->readVRAMBank(tileAddr, 0);
		const uint16 tileDataAddr = TileData::GetAddress(lcd_.tileDataAddress(), tileId, drawingWindow_ ? (windowLine_ % 8) : ((ly + scy) % 8), tileMapAttr.attr.yFlip);

		// タイルデータを参照
		const uint16 tileData = mem_->read16VRAMBank(tileDataAddr, tileMapAttr.attr.bank);
		const uint8 color = TileData::GetColor(tileData, x % 8, tileMapAttr.attr.xFlip);

		// 実際の描画色
		Color dispColor;
		if (not cgbMode_)
			dispColor = displayColorPalette_[(int)lcd_.bgp(color)];
		else
			dispColor = displayBGColorPalette_[tileMapAttr.attr.palette][color];


		// スプライトをフェッチ
		if (lcd_.isEnabledSprite())
		{
			int oamPriorityVal = 999;
			int oamIndex = 0;

			for (const auto& oam : oamBuffer_)
			{
				// 0xff6c(OPRI)を反映
				if (oamPriorityVal != 999 && ((opri && oamPriorityVal < oam.x) || (not opri && oamPriorityVal < oamIndex))) continue;

				// 描画中のドットがスプライトに重なっているか？
				if (not (oam.x <= canvasX_ + 8 && oam.x + 8 > canvasX_ + 8)) continue;

				// スプライトの、左から oamX 個目のドットを描画する
				const int oamX = canvasX_ + 8 - oam.x;

				// タイルデータのアドレスを得る
				const uint16 oamTileDataAddr = TileData::GetAddress(0x8000, oam.tile, (ly + 16 - oam.y) % 8, oam.yFlip);

				// タイルデータを参照
				const uint16 oamTileData = mem_->read16VRAMBank(oamTileDataAddr, oam.bank);
				const uint8 oamColor = TileData::GetColor(oamTileData, oamX % 8, oam.xFlip);

				// BGとのマージ
				if (not cgbMode_)
				{
					if (oamColor != 0 && not (oam.priority == 1 && color != 0))
					{
						dispColor = displayColorPalette_[(int)lcd_.obp(oam.palette, oamColor)];
					}
				}
				else
				{
					// Refer: https://gbdev.io/pandocs/Tile_Maps.html#bg-to-obj-priority-in-cgb-mode

					bool drawObj = false;

					if (oamColor != 0)
					{

						// LCDC.0 :
						// ... When Bit 0 is cleared, ... the sprites will be always displayed on top of background and window
						// independently of the priority flags in OAM and BG Map attributes.
						if (not lcdc0)
						{
							drawObj = true;
						}
						else
						{
							if (oam.priority == 0 && tileMapAttr.attr.priority == 0)
							{
								drawObj = true;
							}
							else
							{
								if (color == 0) drawObj = true;
							}
						}
					}

					if (drawObj)
					{
						dispColor = displayOBJColorPalette_[oam.obp][oamColor];
						oamPriorityVal = opri ? oam.x : oamIndex;
					}
				}

				oamIndex++;
			}
		}

		canvas_[ly][canvasX_] = dispColor;

		fetcherX_++;
		canvasX_++;
	}
}
