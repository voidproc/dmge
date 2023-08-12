#pragma once

namespace dmge
{
	namespace GUI
	{
		class TextboxOverlay
		{
		public:
			TextboxOverlay(StringView defaultText = U"", StringView labelText = U"");

			void update();

			void drawAt(const Vec2& pos) const;

			void show(bool visible);

			bool isVisible() const;

			StringView text() const;

		private:
			String text_{};
			String labelText_{};
			bool visible_ = false;
			int cursorPos_ = 0;
		};
	}
}
