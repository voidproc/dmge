#pragma once

#include "PPUMode.h"
#include "Colors.h"

namespace dmge
{
	class Memory;
	class Interrupt;
	class LCD;
	struct BufferedOAM;

	class PPU
	{
	public:
		PPU(Memory* mem, LCD* lcd, Interrupt* interrupt);

		~PPU();

		void setCGBMode(bool value);

		// このフレームのレンダリングを1ドット進める
		// LY・PPUモード・STATを更新する
		// 状態更新の結果により割り込み要求を行う
		void run();

		// PPUによるレンダリング結果をシーンに描画する
		void draw(const Point& pos, int scale);

		// PPUのモード
		// LYと、このフレームの描画ドット数により変化する
		PPUMode mode() const;

		// PPUのモードが OAM Scan に変化した
		bool modeChangedToOAMScan() const;

		// PPUのモードが HBlank に変化した
		bool modeChangedToHBlank() const;

		// PPUのモードが VBlank に変化した
		bool modeChangedToVBlank() const;

		// このフレームの描画ドット数
		int dot() const;

		// (DMG) カラーパレットを設定する
		// （「色番号から実際の色への変換テーブル」を置き換える）
		void setDisplayColorPalette(const std::array<Color, 4>& palette);

	private:
		Memory* mem_;
		LCD* lcd_;
		Interrupt* interrupt_;

		// このフレームの描画ドット数
		int dot_ = 0;

		// 前回のLY, LYC
		// STAT更新用
		uint8 prevLY_ = 0;
		uint8 prevLYC_ = 0;

		// PPUのモード
		// LYと、このフレームの描画ドット数により変化する
		PPUMode mode_ = PPUMode::OAMScan;

		// 前回STATによる割り込み要求をしたか（してたら今回は要求しない）
		bool prevStatInt_ = false;

		// レンダリング結果
		Image canvas_;
		DynamicTexture texture_;

		// ピクセルフェッチャーのいるX座標
		// ウィンドウフェッチが開始すると0にリセットされる
		uint8 fetcherX_ = 0;

		// レンダリング中のX座標
		// スプライト用。ウィンドウフェッチによりリセットされない
		uint8 canvasX_ = 0;

		// このフレーム内で LY==WY となったことがある（ウィンドウに到達した）
		bool toDrawWindow_ = false;

		// ウィンドウをフェッチ中
		bool drawingWindow_ = false;

		// ウィンドウフェッチを行った行数
		uint8 windowLine_ = 0;

		// OAMバッファ
		// スキャンラインのはじめのあたり（OAMScanモード時）に構築される
		Array<BufferedOAM> oamBuffer_;

		// CGB Mode
		bool cgbMode_ = false;

		// (DMG) 色番号から実際の色への変換テーブル
		std::array<Color, 4> displayColorPalette_{};


		void updateLY_();
		void updateMode_();
		void updateSTAT_();
		void scanOAM_();
		void scanline_();

	};
}
