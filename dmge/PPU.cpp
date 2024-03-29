﻿#include "stdafx.h"
#include "PPU.h"
#include "PPUConstants.h"
#include "LCD.h"
#include "Memory.h"
#include "Interrupt.h"
#include "TileData.h"
#include "BitMask/InterruptFlag.h"
#include "OAM.h"
#include "TileMapAttribute.h"

namespace dmge
{

	namespace
	{
		HLSL PPURenderingShader()
		{
			return HLSL{ Resource(U"shaders/lcd.hlsl"), U"PS" };
		}
	}

	PPU::PPU(Memory* mem, LCD* lcd, Interrupt* interrupt)
		:
		mem_{ mem },
		lcd_{ lcd },
		interrupt_{ interrupt },
		canvas_{ LCDSize.x + 8, LCDSize.y },
		texture_{ canvas_.size() }
	{
		dot_ = FrameDots - 52 + 4;
		canvas_.fill(Palette::White);
		oamBuffer_.reserve(10);

#if SIV3D_PLATFORM(WINDOWS)
		pixelShader_ = HLSL{ PPURenderingShader() };
#endif
	}

	PPU::~PPU()
	{
	}

	void PPU::setCGBMode(bool enableCGBMode)
	{
		cgbMode_ = enableCGBMode;

		cbRenderingSetting_->gamma = enableCGBMode ? gamma_ : 1.0;
	}

	void PPU::setSGBMode(bool enableSGBMode)
	{
		sgbMode_ = enableSGBMode;
	}

	void PPU::setGamma(double gamma)
	{
		gamma_ = gamma;

		if (cgbMode_) cbRenderingSetting_->gamma = gamma;
	}

	void PPU::run()
	{
		// 1ドット進めてLYを更新
		dot_ = (dot_ + 1) % FrameDots;
		updateLY_();

		// PPU Modeを更新
		updateMode_();

		// LCDの状態を更新
		updateSTAT_();

		// 現在の行に存在するOAMを探す
		// OAMScanモードへの移行直後に行うと意図した結果にならない気がするので、モード終盤まで待つ

		if (mode_ == PPUMode::OAMScan && (dot_ % LineDots) == Mode2Dots - 1)
		{
			oamBuffer_.clear();
			scanOAM_();
		}

		// 行の描画

		if (mode_ == PPUMode::Drawing)
		{
			if (canvasX_ < LCDSize.x)
			{
				renderDot_();
			}
		}

		// 右端まで行ったらフェッチャーの状態をリセットして次の行へ

		if (modeChangedToHBlank())
		{
			// 右端の残りのドットを描画
			while (canvasX_ < LCDSize.x)
			{
				renderDot_();
			}

			fetcherX_ = 0;
			canvasX_ = 0;

			if (drawingWindow_)
			{
				windowLine_++;
				drawingWindow_ = false;
			}
		}

		// VBlankに移行したので
		// - このフレームの描画結果を RenderTexture に出力
		// - ピクセルフェッチャーの状態をリセット
		// - 割り込み要求

		if (modeChangedToVBlank())
		{
			flushRenderingResult();

			toDrawWindow_ = false;
			drawingWindow_ = false;
			windowLine_ = 0;

			interrupt_->request(BitMask::InterruptFlagBit::VBlank);
		}

		// STAT割り込み要求

		if (checkChangedSTATSources_())
		{
			interrupt_->request(BitMask::InterruptFlagBit::STAT);
		}
	}

	void PPU::flushRenderingResult()
	{
		if (mask_ != SGB::MaskMode::Freeze)
		{
			texture_.fill(canvas_);
		}
	}

	void PPU::draw(const Vec2& pos, int scale)
	{
		if (not lcd_->isEnabled()) return;

		const ScopedRenderStates2D renderState{ SamplerState::ClampNearest };

		const Transformer2D transformer{ Mat3x2::Scale(scale).translated(pos) };

#if SIV3D_PLATFORM(WINDOWS)
		Graphics2D::SetPSConstantBuffer(1, cbRenderingSetting_);

		const ScopedCustomShader2D shader{ pixelShader_ };
#endif

		texture_(0, 0, 160, 144).draw();
	}

	PPUMode PPU::mode() const
	{
		return mode_;
	}

	bool PPU::modeChangedToOAMScan() const
	{
		return (dot_ < LCDSize.y * LineDots) && ((dot_ % LineDots) == 0);
	}

	bool PPU::modeChangedToHBlank() const
	{
		return (dot_ < LCDSize.y * LineDots) && ((dot_ % LineDots) == Mode2Dots + mode3Length());
	}

	bool PPU::modeChangedToVBlank() const
	{
		return dot_ == LCDSize.y * LineDots;
	}

	bool PPU::checkChangedSTATSources_()
	{
		bool requireSTATInt = false;
		requireSTATInt |= (lcd_->isEnabledLYCInterrupt() && lcd_->lycFlag());
		requireSTATInt |= (lcd_->isEnabledOAMScanInterrupt() && modeChangedToOAMScan());
		requireSTATInt |= (lcd_->isEnabledOAMScanInterrupt() && modeChangedToVBlank()); //OAMScan INT 有効時にも Vblank INT が発火する？
		requireSTATInt |= (lcd_->isEnabledHBlankInterrupt() && modeChangedToHBlank());
		requireSTATInt |= (lcd_->isEnabledVBlankInterrupt() && modeChangedToVBlank());

		const bool changed = requireSTATInt && !prevRequireSTATInt_;

		prevRequireSTATInt_ = requireSTATInt;

		return changed;
	}

	int PPU::dot() const
	{
		return dot_;
	}

	int PPU::mode3Length() const
	{
		const auto baseLength = Mode3DotsShort;

		if (lcd_->ly() >= lcd_->wy()) return baseLength + 8;

		return baseLength;
	}

	void PPU::setPaletteColors(const std::array<ColorF, 4>& palette)
	{
		std::copy(palette.cbegin(), palette.cend(), paletteColors_.begin());
	}

	void PPU::transferAttributeFiles()
	{
		for (uint16 addr = lcd_->tileDataAddressTop(), index = 0; index <= 0xfd1; ++addr, ++index)
		{
			sgbAttrFile_[index] = mem_->read(addr);
		}
	}

	void PPU::setAttribute(int index)
	{
		for (int i : step(90))
		{
			sgbCurrentAttr_[i] = sgbAttrFile_[index * 90 + i];
		}
	}

	void PPU::dumpAttributeFile(int index)
	{
		for (int i : step(90))
		{
			Console.write(U"{:02X} "_fmt(sgbAttrFile_[index * 90 + i]));
		}
		Console.writeln();
	}

	void PPU::updateLY_()
	{
		if (not lcd_->isEnabled())
		{
			dot_ = 0;
		}

		// 4 T-cycles 進めたときに LY + 1 となる状況では LY + 1 となるようにする
		// ただし最終ラインは除く
		int dot = dot_;
		if (dot < FrameDots - LineDots && (dot % LineDots) >= 452) dot += 456 - (dot % LineDots);

		uint8 ly = static_cast<uint8>(dot / LineDots);

		// "scanline 153 quirk"
		if (ly == 153 && (dot % LineDots) >= 4)
		{
			ly = 0;
		}

		lcd_->ly(ly);
	}

	void PPU::updateMode_()
	{
		// Set PPU Mode

		if (dot_ >= LineDots * LCDSize.y)
		{
			mode_ = PPUMode::VBlank;
		}
		else
		{
			if (const int x = dot_ % LineDots; x < Mode2Dots)
			{
				// Mode2 の期間 : 80 dots

				mode_ = PPUMode::OAMScan;
			}
			else if (x < Mode2Dots + mode3Length())
			{
				// Mode3 の期間 : 172 dots
				// ※スプライトの数に応じて可変（172～289）で実装すべきかもしれないが、とりあえず固定で実装
				// ※ライン上にスプライトがないケースがほとんどであると仮定して 172 を使用

				mode_ = PPUMode::Drawing;
			}
			else
			{
				// Mode1 の期間 : 456 - 80 - 172 = 204 dots

				mode_ = PPUMode::HBlank;
			}
		}

		// Set STAT.1-0 (PPU Mode)
		lcd_->setSTATPPUMode(mode_);
	}

	void PPU::updateSTAT_()
	{
		// Set STAT.2 (Coincidence Flag)
		// LYが更新されるかLYCが設定されるときにSTAT.2をセットする

		const uint8 ly = lcd_->ly();
		const uint8 lyc = lcd_->lyc();

		if ((ly != prevLY_) || (lyc != prevLYC_))
		{
			lcd_->setSTATLYCFlag(ly == lyc);
		}

		prevLY_ = ly;
		prevLYC_ = lyc;
	}

	void PPU::scanOAM_()
	{
		const uint8 ly = lcd_->ly();
		const uint8 spriteSize = lcd_->spriteSize();

		// OAMメモリを走査して、描画対象のOAMをバッファに格納していく
		for (uint16 addr = 0xfe00; addr <= 0xfe9f; addr += 4)
		{
			// 1ライン10個まで
			if (oamBuffer_.size() >= 10) break;

			OAM oam{};
			oam.y = mem_->read(addr);

			// Y座標が描画範囲にあるか？
			if (ly + 16 < oam.y) continue;
			if (ly + 16 >= oam.y + spriteSize) continue;

			oam.x = mem_->read(addr + 1);

			// X座標が描画範囲にあるか？
			if (oam.x == 0) continue;

			oam.tile = mem_->read(addr + 2);

			const uint8 flags = mem_->read(addr + 3);
			oam.priority = (flags >> 7) & 1;
			oam.yFlip = (flags >> 6) & 1;
			oam.xFlip = (flags >> 5) & 1;
			oam.palette = (flags >> 4) & 1;
			oam.bank = cgbMode_ ? ((flags >> 3) & 1) : 0;
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

			oam.address = addr;

			oamBuffer_.push_back(oam);
		}

		// DMGの場合、X座標が小さいOBJが優先される。X座標が等しい場合は、先に定義されているものが優先される

		if (not cgbMode_)
		{
			oamBuffer_.sort_by([](const auto& a, const auto& b) { return (a.x == b.x) ? a.address > b.address : a.x > b.x; });
		}
	}

	void PPU::renderDot_()
	{
		const uint8 ly = lcd_->ly();
		const uint8 scy = lcd_->scy();
		const uint8 scx = lcd_->scx();

		uint8 fetcherX = fetcherX_;

		// このフレームでウィンドウに到達したかどうかを toDrawWindow_ に記録する
		if (ly == lcd_->wy())
		{
			toDrawWindow_ = true;
		}

		// このスキャンラインでウィンドウのフェッチが開始したら
		// ピクセルフェッチャーのX座標をリセットする
		// ※スプライトの描画には、リセットされない canvasX_ を使用
		if (not drawingWindow_ && toDrawWindow_ && (fetcherX >= lcd_->wx() - 7))
		{
			if (lcd_->isEnabledWindow())
			{
				drawingWindow_ = true;
				fetcherX_ = fetcherX = 0;
			}
		}

		// タイルマップの中の、描画対象のタイルのアドレスを得る
		uint16 tileAddr;

		if (drawingWindow_)
		{
			const uint16 tileMapAddrBase = lcd_->windowTileMapAddress();
			const uint16 addrOffsetX = fetcherX / 8;
			const uint16 addrOffsetY = 32 * (windowLine_ / 8);
			tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		}
		else
		{
			// BGは、横方向のスクロールの端数分を読み飛ばす
			fetcherX += scx % 8;

			const uint16 tileMapAddrBase = lcd_->bgTileMapAddress();
			const uint16 addrOffsetX = ((fetcherX / 8) + (scx / 8)) & 0x1f;
			const uint16 addrOffsetY = 32 * (((ly + scy) & 0xff) / 8);
			tileAddr = tileMapAddrBase + ((addrOffsetX + addrOffsetY) & 0x3ff);
		}

		// (CGB) 背景マップ属性を取得（VRAM Bank1）
		const TileMapAttribute tileMapAttr{ cgbMode_ ? mem_->readVRAMBank(tileAddr, 1) : uint8(0) };

		// タイルデータのアドレスを得る
		const uint8 tileId = mem_->readVRAMBank(tileAddr, 0);
		const uint16 tileDataAddr = TileData::GetAddress(lcd_->tileDataAddress(), tileId, drawingWindow_ ? (windowLine_ % 8) : ((ly + scy) % 8), tileMapAttr.attr.yFlip);

		// タイルデータを参照
		const uint16 tileData = mem_->read16VRAMBank(tileDataAddr, tileMapAttr.attr.bank);
		const uint8 color = TileData::GetColor(tileData, fetcherX % 8, tileMapAttr.attr.xFlip);

		// 実際の描画色
		Color& dotColor = canvas_[ly][canvasX_];

		if (not cgbMode_)
		{
			// LCDC.0 == 0 の場合はBGを描画しない
			const uint8 bgPaletteColor = lcd_->isEnabledBgAndWindow() ? FromEnum(lcd_->bgp(color)) : 0u;

			if (not sgbMode_)
			{
				dotColor = paletteColors_[bgPaletteColor];
			}
			else
			{
				// (SGB) カラー0は透明なので、最新の背景色を表示する?
				const uint8 palette = bgPaletteColor == 0 ? 0 : getAttribute(canvasX_ / 8, ly / 8);
				dotColor = lcd_->sgbPaletteColor(palette, bgPaletteColor);

				if (mask_ == SGB::MaskMode::Black)
				{
					dotColor = Palette::Black;
				}
				else if (mask_ == SGB::MaskMode::Color0)
				{
					dotColor = lcd_->sgbPaletteColor(0, 0);
				}
			}
		}
		else
		{
			dotColor = lcd_->bgPaletteColor(tileMapAttr.attr.palette, color);
		}

		// スプライトをフェッチ
		if (lcd_->isEnabledSprite())
		{
			dotColor = fetchOAMDot_(dotColor, color, tileMapAttr);
		}

		fetcherX_++;
		canvasX_++;
	}

	ColorF PPU::fetchOAMDot_(const ColorF& initialDotColor, uint8 bgColor, const TileMapAttribute& bgTileMapAttr) const
	{
		const bool opri = lcd_->opri() & 1;
		int oamPriorityVal = 999;
		int oamIndex = 0;

		// 描画結果
		ColorF fetched = initialDotColor;

		for (const auto& oam : oamBuffer_)
		{
			// 描画中のドットがスプライトに重なっているか？
			if (not (oam.x <= canvasX_ + 8 && oam.x + 8 > canvasX_ + 8)) continue;

			// 0xff6c(OPRI)を反映
			if (cgbMode_ && oamPriorityVal != 999 && ((opri && oamPriorityVal < oam.x) || (not opri && oamPriorityVal < oamIndex))) continue;

			// タイルデータのアドレスを得る
			const uint16 tileDataAddr = TileData::GetAddress(0x8000, oam.tile, (lcd_->ly() + 16 - oam.y) % 8, oam.yFlip);

			// タイルデータを参照
			const uint16 tileData = mem_->read16VRAMBank(tileDataAddr, oam.bank);

			// スプライトの、左から oamX 個目のドットを描画する
			const int oamX = canvasX_ + 8 - oam.x;
			const uint8 oamColor = TileData::GetColor(tileData, oamX % 8, oam.xFlip);

			// BGとのマージ
			if (not cgbMode_)
			{
				if (oamColor != 0 && not (oam.priority == 1 && bgColor != 0))
				{
					const uint8 oamPaletteColor = FromEnum(lcd_->obp(oam.palette, oamColor));

					if (not sgbMode_)
					{
						fetched = paletteColors_[oamPaletteColor];
					}
					else
					{
						// (SGB) カラー0は透明なので、最新の背景色を表示する?
						const uint8 palette = oamPaletteColor == 0 ? 0 : getAttribute(canvasX_ / 8, lcd_->ly() / 8);
						fetched = lcd_->sgbPaletteColor(palette, oamPaletteColor);
					}

					if (mask_ == SGB::MaskMode::Black)
					{
						fetched = Palette::Black;
					}
					else if (mask_ == SGB::MaskMode::Color0)
					{
						fetched = lcd_->sgbPaletteColor(0, 0);
					}
				}
			}
			else
			{
				// - https://gbdev.io/pandocs/LCDC.html#lcdc0--bg-and-window-enablepriority
				// - https://gbdev.io/pandocs/Tile_Maps.html#bg-to-obj-priority-in-cgb-mode

				const bool drawObj =
					((oamColor != 0) &&
						((not lcd_->isEnabledBgAndWindow()) ||
							(oam.priority == 0 && bgTileMapAttr.attr.priority == 0) ||
							(bgColor == 0)));

				if (drawObj)
				{
					fetched = lcd_->objPaletteColor(oam.obp, oamColor);
					oamPriorityVal = opri ? oam.x : oamIndex;
				}
			}

			oamIndex++;
		}

		return fetched;
	}

	void PPU::setAttribute(int x, int y, uint8 palette)
	{
		const uint8 index = y * 5 + x / 4;
		const uint8 shiftBits = ((3 - (x % 4)) * 2);

		sgbCurrentAttr_[index] &= ~(0x3 << shiftBits);
		sgbCurrentAttr_[index] |= (palette << shiftBits);
	}

	uint8 PPU::getAttribute(int x, int y) const
	{
		const uint8 index = y * 5 + x / 4;
		const uint8 shiftBits = ((3 - (x % 4)) * 2);

		return (sgbCurrentAttr_[index] >> shiftBits) & 0x3;
	}

	void PPU::setMask(SGB::MaskMode mask)
	{
		mask_ = mask;
	}
}
