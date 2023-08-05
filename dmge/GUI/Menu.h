#pragma once

namespace dmge
{
	struct AppConfig;

	namespace GUI
	{
		// メニューのテキスト
		struct MenuItemText
		{
			String label;
			String state;
			String shortcut;

			String plainText() const;
			String taggedText() const;
		};

		struct MenuItem
		{
			// メニュー項目のテキスト
			MenuItemText text;

			// メニュー項目のテキスト
			// 関数の戻り値がメニュー項目のテキストとして設定される
			std::function<MenuItemText()> textFunc;

			// メニュー項目が決定されたときに行う処理
			std::function<void()> handler;

			// メニュー項目に対し ←、→ キーが押下されたときに行う処理
			// 引数に bool leftPressed (←キーが押されたか) をとる
			std::function<void(bool)> handlerLR;

			bool enableLR = false;

			// パレット選択用
			bool isPaletteSetting = false;
		};

		struct Menu
		{
			Array<MenuItem> items;

			int getTotalHeight() const;
		};

		class MenuOverlay
		{
		public:
			MenuOverlay(AppConfig& config);

			void set(Menu& menu);

			void show();

			void hide();

			bool isVisible() const;

			void update();

			void draw() const;

		private:
			AppConfig& config_;

			bool visible_ = false;

			Menu menu_;

			// 選択されているメニュー項目のインデックス
			int selectedIndex_ = 0;

			// 縦方向のスクロール量
			int verticalScroll_ = 0;

			// マウスオーバーによる選択の有効化
			bool enableMouseSelection_ = true;

			Rect scrollingAreaRect_() const;
			void scrollToIndex_(int index);
			void scroll_(int scrollAmount);
			Transformer2D getScrollingAreaTransform_() const;
			void drawMenuItemTextAt_(const MenuItemText& text, const Vec2& pos, bool selected) const;
			void drawHelp_() const;
		};
	}
}
