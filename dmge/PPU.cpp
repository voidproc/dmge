#include "PPU.h"
#include "LCD.h"
#include "Memory.h"
#include "Interrupt.h"
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

	PPU::PPU(Memory* mem, LCD* lcd, Interrupt* interrupt)
		:
		mem_{ mem },
		lcd_{ lcd },
		interrupt_{ interrupt },
		canvas_{ LCDSize.x + 8, LCDSize.y },
		texture_{ canvas_.size() }
	{
		dot_ = 70224 - 52 + 4;
		canvas_.fill(Palette::White);
		oamBuffer_.reserve(10);
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
		// 1ドット進めてLYを更新
		dot_ = (dot_ + 1) % 70224;
		updateLY_();

		// PPU Modeを更新
		updateMode_();

		// LCDの状態を更新
		updateSTAT_();

		// 現在の行に存在するOAMを探す
		// OAMScanモードへの移行直後に行うと意図した結果にならない気がするので、モード終盤まで待つ

		if (mode_ == PPUMode::OAMScan && (dot_ % 456) == 79)
		{
			oamBuffer_.clear();
			scanOAM_();
		}

		// 行の描画

		if (mode_ == PPUMode::Drawing)
		{
			if (fetcherX_ < LCDSize.x && canvasX_ < LCDSize.x)
			{
				scanline_();
			}
		}

		// 右端まで行ったらフェッチャーの状態をリセットして次の行へ

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

		// VBlank割り込み要求

		if (modeChangedToVBlank())
		{
			interrupt_->request(BitMask::InterruptFlagBit::VBlank);
		}

		// STAT割り込み要求

		bool statInt = false;
		statInt |= (lcd_->isEnabledLYCInterrupt() && lcd_->lycFlag());
		statInt |= (lcd_->isEnabledOAMScanInterrupt() && modeChangedToOAMScan());
		statInt |= (lcd_->isEnabledHBlankInterrupt() && modeChangedToHBlank());
		statInt |= (lcd_->isEnabledVBlankInterrupt() && modeChangedToVBlank());

		if (statInt && !prevStatInt_)
		{
			interrupt_->request(BitMask::InterruptFlagBit::STAT);
		}

		prevStatInt_ = statInt;
	}

	void PPU::draw(const Point& pos, int scale)
	{
		if (not lcd_->isEnabled()) return;

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

	void PPU::updateLY_()
	{
		if (not lcd_->isEnabled())
		{
			dot_ = 0;
		}

		uint8 ly = static_cast<uint8>(dot_ / 456);

		// "scanline 153 quirk"
		if (ly == 153 && (dot_ % 456) >= 4)
		{
			ly = 0;
		}

		lcd_->ly(ly);
	}

	void PPU::updateMode_()
	{
		// Set PPU Mode

		if (dot_ >= 456 * 144)
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

		const uint8 stat = lcd_->stat();
		mem_->write(Address::STAT, 0x80 | (stat & ~3) | (lcd_->isEnabled() ? (uint8)mode_ : 0));
	}

	void PPU::updateSTAT_()
	{
		// Set STAT.2 (Coincidence Flag)
		// LYが更新されるかLYCが設定されるときにSTAT.2をセットする

		const uint8 ly = lcd_->ly();
		const uint8 lyc = lcd_->lyc();

		if ((ly != prevLY_) || lyc != prevLYC_)
		{
			const uint8 stat = lcd_->stat();
			mem_->write(Address::STAT, 0x80 | (stat & ~4) | (ly == lyc ? 4 : 0));
		}

		prevLY_ = ly;
		prevLYC_ = lyc;
	}

	void PPU::scanOAM_()
	{
		const uint8 ly = lcd_->ly();
		const int spriteSize = lcd_->isEnabledTallSprite() ? 16 : 8;

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

		if (not cgbMode_)
		{
			oamBuffer_.sort_by([](const auto& a, const auto& b) { return a.x > b.x; });
		}
	}

	void PPU::scanline_()
	{
		// DMG: BGとWindow無効
		// CGB: BGとWindowの優先度無効
		const bool lcdc0 = lcd_->isEnabledBgAndWindow();

		// (DMG) BGとWindowが無効なので、ピクセルを1つ進めて終了
		if (not cgbMode_ && not lcdc0)
		{
			canvas_[lcd_->ly()][canvasX_] = displayColorPalette_[0];

			fetcherX_++;
			canvasX_++;
			return;
		}

		// 現在の状態
		const uint8 ly = lcd_->ly();
		const uint8 scy = lcd_->scy();
		const uint8 scx = lcd_->scx();
		const uint8 wy = lcd_->wy();
		const uint8 wx = lcd_->wx();
		const bool enabledWindow = lcd_->isEnabledWindow();
		const uint8 opri = lcd_->opri() & 1;

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
			const uint16 tileMapAddrBase = lcd_->windowTileMapAddress();
			const uint16 addrOffsetX = x / 8;
			const uint16 addrOffsetY = 32 * (windowLine_ / 8);
			tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		}
		else
		{
			const uint16 tileMapAddrBase = lcd_->bgTileMapAddress();
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
		const uint16 tileDataAddr = TileData::GetAddress(lcd_->tileDataAddress(), tileId, drawingWindow_ ? (windowLine_ % 8) : ((ly + scy) % 8), tileMapAttr.attr.yFlip);

		// タイルデータを参照
		const uint16 tileData = mem_->read16VRAMBank(tileDataAddr, tileMapAttr.attr.bank);
		const uint8 color = TileData::GetColor(tileData, x % 8, tileMapAttr.attr.xFlip);

		// 実際の描画色
		Color dispColor;
		if (not cgbMode_)
			dispColor = displayColorPalette_[(int)lcd_->bgp(color)];
		else
			dispColor = lcd_->bgPaletteColor(tileMapAttr.attr.palette, color);


		// スプライトをフェッチ
		if (lcd_->isEnabledSprite())
		{
			int oamPriorityVal = 999;
			int oamIndex = 0;

			const auto filteredOAMs = oamBuffer_.filter([&](const auto& o) { return o.x <= canvasX_ + 8 && o.x + 8 > canvasX_ + 8; });

			for (const auto& oam : filteredOAMs)
			{
				// 0xff6c(OPRI)を反映
				if (cgbMode_ && oamPriorityVal != 999 && ((opri && oamPriorityVal < oam.x) || (not opri && oamPriorityVal < oamIndex))) continue;

				// 描画中のドットがスプライトに重なっているか？
				//if (not (oam.x <= canvasX_ + 8 && oam.x + 8 > canvasX_ + 8)) continue;

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
						dispColor = displayColorPalette_[(int)lcd_->obp(oam.palette, oamColor)];
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
						dispColor = lcd_->objPaletteColor(oam.obp, oamColor);
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
