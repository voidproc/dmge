#include "TextboxOverlay.h"

namespace dmge
{
	namespace GUI
	{
		TextboxOverlay::TextboxOverlay(StringView defaultText, StringView labelText)
			: text_{ defaultText }, labelText_{ labelText }, cursorPos_{ (int)defaultText.length() }
		{
		}

		void TextboxOverlay::update()
		{
			if (not visible_) return;

			if (KeyLeft.down())
			{
				cursorPos_ = Max(cursorPos_ - 1, 0);
			}
			else if (KeyRight.down())
			{
				cursorPos_ = Min((int)text_.length(), cursorPos_ + 1);
			}

			cursorPos_ = TextInput::UpdateText(text_, cursorPos_, TextInputMode::AllowBackSpaceDelete);
		}

		void TextboxOverlay::drawAt(const Vec2& pos) const
		{
			if (not visible_) return;

			constexpr ColorF BgColor{ 0, 0.3 };
			constexpr ColorF PanelColor{ 0, 0.8 };
			constexpr ColorF LabelColor{ Palette::White };
			constexpr ColorF TextColor{ Palette::Khaki };
			const ColorF CursorColor = ColorF{ Palette::Khaki, Periodic::Square0_1(0.4s) };
			const Size PanelSize{ Scene::Width(), 80 };

			// 背景
			Scene::Rect().draw(BgColor);

			// パネル
			RectF{ PanelSize }.setCenter(pos).draw(PanelColor);

			// テキスト（"Address:"）
			FontAsset(U"textbox")(labelText_).drawAt(pos.movedBy(0, -12), LabelColor);

			// テキスト（入力テキスト）

			const auto region = FontAsset(U"textbox")(text_).regionAt(pos.movedBy(0, 12));
			Vec2 penPos{ region.tl() };
			Vec2 editingTextPos = penPos;

			for (auto [indexChar, glyph] : Indexed(FontAsset(U"textbox").getGlyphs(text_)))
			{
				const auto glyphRect = glyph.texture.draw(penPos + glyph.getOffset(), TextColor);

				// カーソル
				if (cursorPos_ == indexChar)
				{
					glyphRect.left().draw(2, CursorColor);
					editingTextPos = penPos;
				}

				penPos.x += glyph.xAdvance;
			}

			// カーソル
			if (cursorPos_ == text_.length())
			{
				region.right().draw(2, CursorColor);
				editingTextPos = region.tr();
			}

			// 未変換のテキスト
			if (const auto editingText = TextInput::GetEditingText(); not editingText.isEmpty())
			{
				const auto t = FontAsset(U"textbox")(editingText);
				const auto textRect = t.region(editingTextPos);
				textRect.draw(ColorF{ 0, 0.5 });
				t.draw(editingTextPos, Palette::Cyan);
			}
		}

		void TextboxOverlay::show(bool visible)
		{
			visible_ = visible;
		}

		bool TextboxOverlay::isVisible() const
		{
			return visible_;
		}

		StringView TextboxOverlay::text() const
		{
			return text_;
		}
	}
}
