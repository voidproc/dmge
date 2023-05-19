﻿#include "PPU.h"
#include "LCD.h"
#include "BitMask/InterruptFlag.h"

namespace dmge
{
	PPU::PPU(Memory* mem, LCD* lcd)
		: mem_{ mem }, lcd_{ lcd }
	{
		dot_ = 70224 - 52 + 4;

		canvas_.resize(160 + 8, 144, Palette::White);
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
		if (mode_ == PPUMode::Drawing && fetcherX_ < 160 && canvasX_ < 160)
		{
			scanline_();
		}
		if (modeChangedToHBlank())
		{
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
		if (lcd_->isEnabledLYCInterrupt() && lcd_->lycFlag())
		{
			statInt = true;
		}
		if (lcd_->isEnabledOAMScanInterrupt() && modeChangedToOAMScan())
		{
			statInt = true;
		}
		if (lcd_->isEnabledHBlankInterrupt() && modeChangedToHBlank())
		{
			statInt = true;
		}
		if (lcd_->isEnabledVBlankInterrupt() && modeChangedToVBlank())
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

	void PPU::draw(const Point pos, int scale)
	{
		if (not lcd_->isEnabled()) return;

		{
			Graphics2D::SetScissorRect(Rect{ pos, Size{ 160, 144 } * scale });

			RasterizerState rs = RasterizerState::Default2D;
			rs.scissorEnable = true;
			const ScopedRenderStates2D renderStates{ rs };

			const Transformer2D transformer{ Mat3x2::Scale(scale).translated(pos) };

			for (int y : step(144))
			{
				for (int x : step(160 + 8))
				{
					Rect{ x, y, 1, 1 }.draw(canvas_.at(y, x));
				}
			}
		}
	}

	void PPU::scanline_()
	{
		if (not lcd_->isEnabledBgAndWindow())
		{
			fetcherX_++;
			canvasX_++;
			return;
		}

		const uint8 ly = lcd_->ly();
		const uint8 scy = lcd_->scy();
		const uint8 scx = lcd_->scx();
		const uint8 wy = lcd_->wy();
		const uint8 wx = lcd_->wx();
		const bool enabledWindow = lcd_->isEnabledWindow();
		const uint16 tileDataAddrBase = lcd_->tileDataAddress();
		const auto bgp = lcd_->bgp();
		const auto obp = lcd_->obp();

		int x = fetcherX_;

		if (ly == wy)
		{
			toDrawWindow_ = true;
		}

		if (not drawingWindow_ && enabledWindow && toDrawWindow_ && (x >= wx - 7))
		{
			drawingWindow_ = true;
			x = fetcherX_ = 0;
		}

		const uint16 tileMapAddrBase = drawingWindow_ ? lcd_->windowTileMapAddress() : lcd_->bgTileMapAddress();

		const uint16 addrOffsetX = drawingWindow_ ?
			x / 8 :
			((x / 8) + (scx / 8)) & 0x1f;
		const uint16 addrOffsetY = drawingWindow_ ?
			32 * (windowLine_ / 8) :
			32 * (((ly + scy) & 0xff) / 8);
		const uint16 tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		const uint8 tileId = mem_->read(tileAddr);

		const uint16 tileDataAddr = tileDataAddrBase + (tileDataAddrBase == 0x8000 ? tileId : (int8)tileId) * 0x10;
		const uint16 tileDataAddrOffset = 2 * ((drawingWindow_ ? windowLine_ : (ly + scy)) % 8);
		const uint8 tileLo = mem_->read(tileDataAddr + tileDataAddrOffset);
		const uint8 tileHi = mem_->read(tileDataAddr + tileDataAddrOffset + 1);

		const int shift = 7 - (x % 8);
		uint8 color = ((tileLo >> shift) & 1) | (((tileHi >> shift) & 1) << 1);
		Color dispColor = paletteColorMap_.at(bgp[color]);

		// スプライトをフェッチ
		for (const auto& oam : oamBuffer_)
		{
			if (not lcd_->isEnabledSprite()) break;

			if (oam.x <= canvasX_ + 8 && oam.x + 8 > canvasX_ + 8)
			{
				const uint8 oamX = canvasX_ + 8 - oam.x;
				const int oamShift = oam.xFlip ? (oamX % 8) : (7 - (oamX % 8));
				const uint8 oamTileLo = mem_->read(0x8000 + oam.tile * 0x10 + 2 * (oam.yFlip ? (7 - (ly % 8)) : (ly % 8)));
				const uint8 oamTileHi = mem_->read(0x8000 + oam.tile * 0x10 + 2 * (oam.yFlip ? (7 - (ly % 8)) : (ly % 8)) + 1);
				const uint8 oamColor = ((oamTileLo >> oamShift) & 1) | (((oamTileHi >> oamShift) & 1) << 1);

				if (oamColor != 0 && not (oam.priority == 1 && color != 0))
				{
					dispColor = paletteColorMap_.at(obp[oamColor + oam.palette * 4]);
				}
			}
		}

		canvas_.at(ly, canvasX_) = dispColor;

		fetcherX_++;
		canvasX_++;
	}

	void PPU::scanOAM_()
	{
		const uint8 ly = lcd_->ly();

		// X座標が同じOAMをバッファに追加しないよう、
		// バッファに追加したOAMのX座標を記録しておく
		Array<uint8> xList;

		// OAMメモリを走査して、描画対象のOAMをバッファに格納していく
		for (uint16 addr = 0xfe00; addr <= 0xfe9f; addr += 4)
		{
			if (oamBuffer_.size() >= 10) break;

			BufferedOAM oam;
			oam.y = mem_->read(addr);
			oam.x = mem_->read(addr + 1);

			if (oam.x == 0) continue;
			if (ly + 16 < oam.y) continue;
			if (ly + 16 >= oam.y + (lcd_->isEnabledTallSprite() ? 16 : 8)) continue;

			// 同じX座標のOAMを複数描画しない
			if (xList.contains(oam.x)) continue;
			xList << oam.x;

			oam.tile = mem_->read(addr + 2);
			const uint8 flags = mem_->read(addr + 3);
			oam.priority = (flags >> 7) & 1;
			oam.yFlip = (flags >> 6) & 1;
			oam.xFlip = (flags >> 5) & 1;
			oam.palette = (flags >> 4) & 1;
			oam.cgbFlag = flags & 0b1111;

			if (lcd_->isEnabledTallSprite())
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

	bool PPU::modeChangedToOAMScan() const
	{
		return (dot_ < 144 * 456) && ((dot_ % 456) == 0);
	}

	bool PPU::modeChangedToHBlank() const
	{
		return (dot_ < 144 * 456) && ((dot_ % 456) == 370);
	}

	bool PPU::modeChangedToVBlank() const
	{
		return dot_ == 144 * 456;
	}

	void PPU::updateLY_()
	{
		if (not lcd_->isEnabled())
		{
			dot_ = 0;
		}

		uint8 ly = dot_ / 456;
		lcd_->ly(ly);
	}

	void PPU::updateSTAT_()
	{
		// Set STAT.2 (Coincidence Flag)
		// LYが更新されるかLYCが設定されるときにSTAT.2をセットする

		const bool changedLy = dot_ % 456 == 0;
		const uint8 lyc = lcd_->lyc();

		if (changedLy || lyc != prevLYC_)
		{
			const uint8 ly = lcd_->ly();
			const uint8 stat = lcd_->stat();
			mem_->write(Address::STAT, (stat & ~4) | (ly == lyc ? 4 : 0));
		}

		prevLYC_ = lyc;
	}

	void PPU::updateMode_()
	{
		// Set PPU Mode

		if (lcd_->ly() > 143)
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

		const uint8 stat = lcd_->stat();
		mem_->write(Address::STAT, (stat & ~3) | (uint8)mode_);
	}
}
