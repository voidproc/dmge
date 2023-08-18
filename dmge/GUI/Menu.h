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

			// トグルなど、値の増減ではないが、左右キーでの操作を可能にする
			bool allowLR = false;

			// パレット選択用
			bool isPaletteSetting = false;
		};

		// メニュー項目がマウスオーバー or クリックされたかどうかを表す
		enum class MenuItemMouseEventType
		{
			None,
			Over,
			Clicked,
		};

		struct MenuItemMouseEvent
		{
			MenuItemMouseEventType type;
			int index;
			bool left;
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

			void backToPrevious();

			void show();

			void hide();

			bool isVisible() const;

			void update();

			void draw() const;

		private:
			AppConfig& config_;

			bool visible_ = false;

			Array<Menu> menu_;

			// 選択されているメニュー項目のインデックス
			int selectedIndex_ = 0;

			// 縦方向のスクロール量
			int verticalScroll_ = 0;

			// マウスオーバーによる選択の有効化
			bool enableMouseSelection_ = true;

			Optional<Vec2> swipeBeginPos_;//スワイプ開始時のマウス位置
			int swipeBeginScroll_ = 0;//スワイプ開始時のスクロール量

			Menu& currentMenu_();
			const Menu& currentMenu_() const;

			Rect scrollingAreaRect_() const;
			void scrollToIndex_(int index);
			void scroll_(int scrollAmount);
			Transformer2D getScrollingAreaTransform_() const;
			void drawMenuItemTextAt_(const MenuItemText& text, const Vec2& pos, bool selected) const;
			void drawHelp_() const;
			MenuItemMouseEvent getMenuItemMouseEvent_() const;
		};
	}
}
