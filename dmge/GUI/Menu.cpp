#include "Menu.h"
#include "../AppConfig.h"
#include "../AppWindow.h"
#include "../Colors.h"

namespace dmge
{
	namespace GUI
	{
		namespace
		{
			// メニューの色

			constexpr ColorF OverlayBgColor{ 0, 0.75 };
			constexpr ColorF TitleBgColor{ 0, 0.95 };
			constexpr ColorF HelpBgColor{ 0, 0.95 };
			constexpr ColorF HelpFontColor{ Palette::Darkgray };
			constexpr ColorF MenuItemFontColor{ 0.9 };
			constexpr ColorF MenuItemSelectedFontColor{ Palette::Khaki };
			constexpr ColorF MenuItemValueFontColor{ Palette::Limegreen };
			constexpr ColorF ShadowColor{ 0.2, 0.8 };

			// メニューのサイズ
			constexpr int RowHeight = 24;

			// メニュー表示時のウィンドウの最小サイズ
			constexpr Size MinWindowSize{ 320, RowHeight * 12 };
		}

		String MenuItemText::plainText() const
		{
			return U"{}{}{}"_fmt(label, state.isEmpty() ? U"" : U" : ", state);
		}

		String MenuItemText::taggedText() const
		{
			return U"{}{}[{}]"_fmt(label, state.isEmpty() ? U"" : U" : ", state);
		}

		int Menu::getTotalHeight() const
		{
			return items.size() * RowHeight;
		}

		MenuOverlay::MenuOverlay(AppConfig& config)
			: config_{ config }
		{
		}

		void MenuOverlay::set(Menu& menu)
		{
			menu_.push_back(menu);
			selectedIndex_ = 0;
			scrollToIndex_(0);
		}

		void MenuOverlay::backToPrevious()
		{
			menu_.pop_back();
			selectedIndex_ = 0;
			scrollToIndex_(0);
		}

		void MenuOverlay::show()
		{
			visible_ = true;

			// メニューがはみ出ない程度にウィンドウをリサイズする

			const auto windowSize = Size{ Max(Scene::Width(), MinWindowSize.x), Max(Scene::Height(), MinWindowSize.y) };

			Scene::Resize(windowSize);
			Window::Resize(windowSize, Centering::No);
		}

		void MenuOverlay::hide()
		{
			visible_ = false;

			// メニュー用のウィンドウのリサイズを元に戻す
			dmge::SetScaleWindowSize(config_.scale, config_.showDebugMonitor, Centering::No);
		}

		bool MenuOverlay::isVisible() const
		{
			return visible_;
		}

		void MenuOverlay::update()
		{
			if (not visible_) return;
			if (currentMenu_().items.isEmpty()) return;

			// マウスを動かすと、マウスによる選択が可能になる（キー操作すると、無効になる）
			if (Cursor::Delta().length() > 0)
			{
				enableMouseSelection_ = true;
			}

			// ↑↓キー: １つ前/後の項目を選択し、選択項目を中心にスクロールする

			if (KeyDown.down())
			{
				selectedIndex_ = (selectedIndex_ + 1) % currentMenu_().items.size();
				scrollToIndex_(selectedIndex_);
				enableMouseSelection_ = false;
				return;
			}

			if (KeyUp.down())
			{
				selectedIndex_ = (selectedIndex_ + currentMenu_().items.size() - 1) % currentMenu_().items.size();
				scrollToIndex_(selectedIndex_);
				enableMouseSelection_ = false;
				return;
			}

			// ←→キー: 値の変更

			if (KeyLeft.down() || KeyRight.down())
			{
				if (currentMenu_().items[selectedIndex_].handlerLR)
				{
					// handlerLR が設定されていたら実行
					currentMenu_().items[selectedIndex_].handlerLR(KeyLeft.pressed());
					return;
				}
				else if (currentMenu_().items[selectedIndex_].enableLR)
				{
					// enableLR が設定されていたら、handler を代わりに実行
					if (currentMenu_().items[selectedIndex_].handler)
					{
						currentMenu_().items[selectedIndex_].handler();
					}
					return;
				}
			}

			// Enterキー: 選択されているメニュー項目を決定または値の変更

			if (KeyEnter.up())
			{
				if (currentMenu_().items[selectedIndex_].handler)
				{
					currentMenu_().items[selectedIndex_].handler();
				}
				return;
			}

			// Escキー/マウス右クリック: 前のメニューへ／メニューを閉じる

			if (KeyEscape.up() || MouseR.up())
			{
				if (menu_.size() > 1)
				{
					backToPrevious();
				}
				else
				{
					hide();
				}
				return;
			}

			// マウスホイール: 上下スクロール

			if (Mouse::Wheel())
			{
				scroll_(Mouse::Wheel() * RowHeight * 3);
				return;
			}

			// マウス左クリック: マウスオーバーしているメニュー項目を決定または値の変更

			if (const auto scrollingArea = scrollingAreaRect_();
				scrollingArea.contains(Cursor::Pos()))
			{
				// マウスクリック位置にスクロールを適用したいので座標変換をする
				const Transformer2D transformer = getScrollingAreaTransform_();

				for (auto [index, item] : Indexed(currentMenu_().items))
				{
					const Rect itemRegion{ 0, index * RowHeight, scrollingArea.w, RowHeight };

					if (itemRegion.leftClicked())
					{
						if (currentMenu_().items[selectedIndex_].handler)
						{
							// handler() には座標変換を適用したくない
							goto callSelectedItemHandler;
						}
						return;
					}
					else if (itemRegion.mouseOver() && enableMouseSelection_)
					{
						selectedIndex_ = index;
						return;
					}
				}
			}

			return;

		callSelectedItemHandler:
			currentMenu_().items[selectedIndex_].handler();
		}

		void MenuOverlay::draw() const
		{
			if (not visible_) return;
			if (currentMenu_().items.isEmpty()) return;

			const auto scrollingArea = scrollingAreaRect_();

			// 背景

			scrollingArea.draw(OverlayBgColor);
			Rect{ 0, 0, scrollingArea.w, RowHeight * 2 }.draw(TitleBgColor);
			Rect{ scrollingArea.bl(), scrollingArea.w, RowHeight * 2 }.draw(HelpBgColor);

			// 画面下部の操作説明
			drawHelp_();

			// メニュー項目
			{
				const ScopedViewport2D viewport{ scrollingArea };

				{
					const Transformer2D transformer = getScrollingAreaTransform_();

					for (auto [index, item] : Indexed(currentMenu_().items))
					{
						const bool selected = index == selectedIndex_;

						// メニュー項目の背景色（選択色）

						const Rect itemRegion{ 0, index * RowHeight, scrollingArea.w, RowHeight };
						const ColorF menuItemBgColor = ColorF{ MenuItemSelectedFontColor, selected ? 0.2 : 0 };
						const ColorF menuItemFontColor = selected ? MenuItemSelectedFontColor : MenuItemFontColor;
						itemRegion.draw(menuItemBgColor);

						// メニュー項目のテキストを1文字ずつ描画

						const MenuItemText text = item.textFunc ? item.textFunc() : item.text;

						if (item.isPaletteSetting)
						{
							constexpr int PaletteBoxSize = 14;
							const auto textRegion = FontAsset(U"menu")(text.plainText()).regionAt(itemRegion.center());
							const auto textRegionShifted = textRegion.movedBy(-PaletteBoxSize * 4 / 2, 0);

							drawMenuItemTextAt_(text, textRegionShifted.center(), selected);

							// 現在のパレットカラー

							const auto& palette = config_.palettePreset == 0 ? config_.paletteColors : Colors::PalettePresets[config_.palettePreset];

							for (int i = 0; i < 4; ++i)
							{
								const auto box = RectF{ PaletteBoxSize }.setCenter(textRegionShifted.rightCenter().movedBy(PaletteBoxSize / 2 + (PaletteBoxSize - 1) * i, 0));
								box.movedBy(1, 1).draw(ShadowColor);
								box.draw(palette[i]).drawFrame(1, 0, menuItemFontColor);
							}
						}
						else
						{
							drawMenuItemTextAt_(text, itemRegion.center(), selected);
						}
					}
				}

				// 矢印

				if (verticalScroll_ > 0)
				{
					FontAsset(U"menu")(U"▲").drawAt(scrollingArea.w - 8, RowHeight / 2, AlphaF(0.8 + 0.1 * Periodic::Square0_1(0.5s)));
				}

				if (verticalScroll_ < currentMenu_().getTotalHeight() - scrollingArea.h)
				{
					FontAsset(U"menu")(U"▼").drawAt(scrollingArea.w - 8, scrollingArea.h - RowHeight / 2, AlphaF(0.8 + 0.1 * Periodic::Square0_1(0.5s)));
				}
			}
		}

		Menu& MenuOverlay::currentMenu_()
		{
			return menu_.back();
		}

		const Menu& MenuOverlay::currentMenu_() const
		{
			return menu_.back();
		}

		Rect MenuOverlay::scrollingAreaRect_() const
		{
			return { 0, RowHeight * 2, Scene::Width(), Scene::Height() - RowHeight * 4 };
		}

		void MenuOverlay::scroll_(int scrollAmount)
		{
			verticalScroll_ = Clamp<int>(
				verticalScroll_ + scrollAmount,
				0,
				currentMenu_().getTotalHeight() - scrollingAreaRect_().h);
		}

		void MenuOverlay::scrollToIndex_(int index)
		{
			const auto scrollingArea = scrollingAreaRect_();

			verticalScroll_ = Clamp<int>(
				index * RowHeight + RowHeight / 2 - scrollingArea.h / 2,
				0,
				currentMenu_().getTotalHeight() - scrollingArea.h);
		}

		Transformer2D MenuOverlay::getScrollingAreaTransform_() const
		{
			const auto scrollingArea = scrollingAreaRect_();
			const auto MenuHeight = currentMenu_().getTotalHeight();
			const bool scrollNeeded = scrollingArea.h < MenuHeight;
			const double cameraTranslateY = scrollNeeded ? -verticalScroll_ : (scrollingArea.h - MenuHeight) / 2;
			const auto cameraTransform = Mat3x2::Translate(Float2{ 0, cameraTranslateY });

			return Transformer2D{ cameraTransform, cameraTransform.translated(scrollingArea.pos) };
		}

		void MenuOverlay::drawMenuItemTextAt_(const MenuItemText& text, const Vec2& pos, bool selected) const
		{
			const ColorF menuItemFontColor = selected ? MenuItemSelectedFontColor : MenuItemFontColor;
			ColorF charColor = menuItemFontColor;

			const auto taggedText = text.taggedText();
			const auto region = FontAsset(U"menu")(taggedText).regionAt(pos);
			Vec2 penPos{ region.tl() };

			for (auto [indexChar, glyph] : Indexed(FontAsset(U"menu").getGlyphs(taggedText)))
			{
				if (glyph.codePoint == U'[')
				{
					charColor = MenuItemValueFontColor;
					continue;
				}
				else if (glyph.codePoint == U']')
				{
					charColor = menuItemFontColor;
					continue;
				}

				glyph.texture.draw(penPos + glyph.getOffset().movedBy(1, 1), ShadowColor);
				glyph.texture.draw(penPos + glyph.getOffset(), charColor);

				penPos.x += glyph.xAdvance;
			}
		}

		void MenuOverlay::drawHelp_() const
		{
			constexpr std::array<StringView, 2> helpText = {
				U"[Enter][L-Click]:Select/Change, [←][→]:Change"_sv,
				U"[ESC][R-Click]:Back"_sv,
			};

			for (const auto [index, text] : Indexed(helpText))
			{
				FontAsset(U"menu")(text)
					.drawAt(scrollingAreaRect_().bottomCenter().movedBy(0, RowHeight / 2 + index * RowHeight), HelpFontColor);
			}
		}

	}
}
