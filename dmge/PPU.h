#pragma once

#include "PPUMode.h"
#include "Colors.h"
#include "SGB/Mask.h"

namespace dmge
{
	class Memory;
	class Interrupt;
	class LCD;
	struct OAM;
	union TileMapAttribute;

	// ピクセルシェーダに渡すパラメータ
	struct RenderingSetting
	{
		float gamma = 1;
	};

	class PPU
	{
	public:
		PPU(Memory* mem, LCD* lcd, Interrupt* interrupt);

		~PPU();

		void setCGBMode(bool enableCGBMode);

		void setSGBMode(bool enableSGBMode);

		void setGamma(double gamma);

		// このフレームのレンダリングを1ドット進める
		// LY・PPUモード・STATを更新する
		// 状態更新の結果により割り込み要求を行う
		void run();

		// レンダリング結果をRenderTextureに記録
		void flushRenderingResult();

		// PPUによるレンダリング結果をシーンに描画する
		void draw(const Vec2& pos, int scale);

		// PPUのモード
		// LYと、このフレームの描画ドット数により変化する
		PPUMode mode() const;

		// PPUのモードが OAM Scan に変化した
		bool modeChangedToOAMScan() const;

		// PPUのモードが HBlank に変化した
		bool modeChangedToHBlank() const;

		// PPUのモードが VBlank に変化した
		bool modeChangedToVBlank() const;

		// STAT 割り込みソースが前回から変化したかを記録して返す
		bool checkChangedSTATSources_();

		// このフレームの描画ドット数
		int dot() const;

		int mode3Length() const;

		// (DMG) パレットの4色を設定する
		// （「色番号から実際の色への変換テーブル」を置き換える）
		void setPaletteColors(const std::array<ColorF, 4>& palette);

		// (SGB) ATTR_TRN
		void transferAttributeFiles();

		// (SGB) アトリビュートを設定（Attr files から）
		void setAttribute(int index);

		// (SGB) アトリビュートを設定
		// x, y はキャラクタ単位の座標
		void setAttribute(int x, int y, uint8 palette);

		// (SGB) アトリビュートを取得
		// x, y はキャラクタ単位の座標
		uint8 getAttribute(int x, int y) const;

		// (SGB) マスクを設定
		void setMask(SGB::MaskMode mask);

		// [DEBUG]
		void dumpAttributeFile(int index);

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
		bool prevRequireSTATInt_ = false;

		// レンダリング結果
		Image canvas_;
		DynamicTexture texture_;

		const PixelShader pixelShader_;
		ConstantBuffer<RenderingSetting> cbRenderingSetting_{};

		double gamma_ = 1;

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
		Array<OAM> oamBuffer_;

		// CGB Mode
		bool cgbMode_ = false;

		// SGB Mode
		bool sgbMode_ = false;

		// (DMG) 色番号から実際の色への変換テーブル
		std::array<ColorF, 4> paletteColors_{};

		// (SGB) Attribute Files
		std::array<uint8, 4050> sgbAttrFile_{};

		// (SGB) 現在のアトリビュート
		std::array<uint8, 90> sgbCurrentAttr_{};

		// (SGB) マスク(MASK_EN)の状態
		SGB::MaskMode mask_ = SGB::MaskMode::None;


		void updateLY_();
		void updateMode_();
		void updateSTAT_();
		void scanOAM_();
		void renderDot_();
		ColorF fetchOAMDot_(const ColorF& initialDotColor, uint8 bgColor, const TileMapAttribute& bgTileMapAttr) const;

	};
}
