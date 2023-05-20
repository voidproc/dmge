﻿#include "PPU.h"
#include "LCD.h"
#include "TileData.h"
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

		uint8 ly = static_cast<uint8>(dot_ / 456);
		lcd_->ly(ly);
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

			BufferedOAM oam;
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
			oam.cgbFlag = flags & 0b1111;

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
		// BGとWindowが無効なので、ピクセルを1つ進めて終了
		if (not lcd_->isEnabledBgAndWindow())
		{
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

		// タイルデータのアドレスを得る
		const uint8 tileId = mem_->read(tileAddr);
		const uint16 tileDataAddr = TileData::GetAddress(lcd_->tileDataAddress(), tileId, drawingWindow_ ? (windowLine_ % 8) : ((ly + scy) % 8));

		// タイルデータを参照
		const uint16 tileData = mem_->read16(tileDataAddr);
		const uint8 color = TileData::GetColor(tileData, x % 8);

		// 実際の描画色
		Color dispColor = displayColorPalette_[(int)lcd_->bgp(color)];

		// スプライトをフェッチ
		if (lcd_->isEnabledSprite())
		{
			for (const auto& oam : oamBuffer_)
			{
				// 描画中のドットがスプライトに重なっているか？
				if (not (oam.x <= canvasX_ + 8 && oam.x + 8 > canvasX_ + 8)) continue;

				// スプライトの、左から oamX 個目のドットを描画する
				const int oamX = canvasX_ + 8 - oam.x;

				// タイルデータのアドレスを得る
				const uint16 oamTileDataAddr = TileData::GetAddress(0x8000, oam.tile, ly % 8, oam.yFlip);

				// タイルデータを参照
				const uint16 oamTileData = mem_->read16(oamTileDataAddr);
				const uint8 oamColor = TileData::GetColor(oamTileData, oamX % 8, oam.xFlip);

				// BGとのマージ
				if (oamColor != 0 && not (oam.priority == 1 && color != 0))
				{
					dispColor = displayColorPalette_[(int)lcd_->obp(oam.palette, oamColor)];
				}
			}
		}

		canvas_.at(ly, canvasX_) = dispColor;

		fetcherX_++;
		canvasX_++;
	}
}
