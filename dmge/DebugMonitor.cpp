#include "DebugMonitor.h"
#include "Memory.h"
#include "CPU.h"
#include "Audio/APU.h"
#include "Address.h"
#include "Interrupt.h"
#include "Cartridge.h"

namespace dmge
{
	namespace
	{
		constexpr Color TextColor{ 210 };
		constexpr Color SectionColor = Palette::Khaki;
		constexpr Size FontSize{ 5, 10 };
		constexpr int LineHeight = FontSize.y + 1;
		constexpr double ColumnWidth = FontSize.x * 30;
		constexpr Vec2 Padding{ 4, 2 };

		String Uint8ToHexAndBin(const uint8 num)
		{
			return U"{:02X} ({:08b})"_fmt(num, num);
		}
	}


	class DrawDebugItem
	{
	public:
		DrawDebugItem(const Vec2& initPos)
			: pos_{ initPos }
		{
		}

		void drawSection(StringView sectionName)
		{
			FontAsset(U"debug")(U"[{}]"_fmt(sectionName)).draw(FontSize.y, pos_, SectionColor);

			pos_.y += LineHeight;
		}

		void drawLabelAndValue(StringView label, StringView value)
		{
			FontAsset(U"debug")(U"{} = {}"_fmt(label, value)).draw(FontSize.y, pos_, TextColor);

			pos_.y += LineHeight;
		}

		void drawText(StringView text)
		{
			FontAsset(U"debug")(text).draw(FontSize.y, pos_, TextColor);

			pos_.y += LineHeight;
		}

		void drawEmptyLine(int nLine = 1)
		{
			pos_.y += LineHeight * nLine;
		}

		void drawMemoryDump(Memory* mem, uint16 addr)
		{
			for (uint16 row = 0; row < 4u; row++)
			{
				FontAsset(U"debug")(U"{:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} | {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}"_fmt(
					mem->read(addr + row * 16 + 0),
					mem->read(addr + row * 16 + 1),
					mem->read(addr + row * 16 + 2),
					mem->read(addr + row * 16 + 3),
					mem->read(addr + row * 16 + 4),
					mem->read(addr + row * 16 + 5),
					mem->read(addr + row * 16 + 6),
					mem->read(addr + row * 16 + 7),
					mem->read(addr + row * 16 + 8),
					mem->read(addr + row * 16 + 9),
					mem->read(addr + row * 16 + 10),
					mem->read(addr + row * 16 + 11),
					mem->read(addr + row * 16 + 12),
					mem->read(addr + row * 16 + 13),
					mem->read(addr + row * 16 + 14),
					mem->read(addr + row * 16 + 15)))
					.draw(FontSize.y, pos_, TextColor);

				pos_.y += LineHeight;
			}
		}

		void drawChannelsAmplitude(APU* apu)
		{
			const auto ampList = apu->getAmplitude();

			for (const auto [index, amp] : Indexed(ampList))
			{
				const double amp01 = amp / 15.0;

				constexpr SizeF rectSize{ 6.0, LineHeight - 2 };
				constexpr double xPadding = 3.0;
				const Vec2 bottomLeft{ pos_.x + index * (rectSize.x + xPadding), pos_.y + LineHeight };
				const RectF rect{ Arg::bottomLeft = bottomLeft, rectSize };

				if (apu->getMute(index))
				{
					rect.draw(ColorF{ 0.05 }).drawFrame(0, 1, ColorF{ Palette::Red, 0.5 });
				}
				else
				{
					rect.draw(ColorF{ 1.0, 0.05 + 0.3 * amp01 });
					rect.scaledAt(bottomLeft, 1.0, amp01).draw(ColorF{ 1.0, 1 - 0.5 * amp01 });
				}
			}

			pos_.y += LineHeight;
		}

		void drawTileDataTexture(const TileDataTexture& tileDataTexture)
		{
			tileDataTexture.draw(pos_.movedBy(0, 2))
				.drawFrame(0, 1, ColorF{ 0.5 });

			pos_.y += LineHeight * (tileDataTexture.size().y / LineHeight + 1);
		}

	private:
		Vec2 pos_{};
	};


	DebugMonitor::DebugMonitor(Memory* mem, CPU* cpu, APU* apu, Interrupt* interrupt)
		: mem_{ mem }, cpu_{ cpu }, apu_{ apu }, interrupt_{ interrupt }, tileDataTexture_{ *mem }, tileDataTextureCGB_{ *mem, 1 }
	{
	}

	DebugMonitor::~DebugMonitor()
	{
	}

	void DebugMonitor::update()
	{
		const bool showDumpAddressTextbox = textbox_.isVisible();

		// テキストボックス表示 (Ctrl+M)
		if (not showDumpAddressTextbox && (KeyM.down() && KeyControl.pressed()))
		{
			textbox_.show(true);
		}

		if (showDumpAddressTextbox)
		{
			// テキストボックス消去 (Enter or ESC or Ctrl+M)
			if (KeyEnter.down() || KeyEscape.down() || (KeyM.down() && KeyControl.pressed()))
			{
				if (TextInput::GetEditingText().isEmpty())
				{
					textbox_.show(false);
					timerTextboxHidden_.restart();
				}
			}

			// テキスト入力を処理
			textbox_.update();

			if (const auto addr = ParseIntOpt<uint16>(textbox_.text(), 16);
				addr)
			{
				dumpAddress_ = Min<uint16>(*addr, 0xffc0);
			}
		}

		static int cnt = 0;
		if (cnt++ % 16 == 0)
		{
			if (mem_->isVRAMTileDataModified())
			{
				tileDataTexture_.update();

				if (mem_->isCGBMode())
				{
					tileDataTextureCGB_.update();
				}

				mem_->resetVRAMTileDataModified();
			}
		}
	}

	void DebugMonitor::draw(const Point& pos) const
	{
		{
			const ScopedViewport2D viewport{ pos.x, pos.y, ViewportSize.x, Scene::Height() };

			Scene::Rect().draw(BgColor);

			{
				DrawDebugItem d{ Vec2{ ColumnWidth * 0, 0 } + Padding };

				// Cartridge

				d.drawSection(U"Cartridge");
				d.drawText(U"{} mode"_fmt(mem_->isCGBMode() ? U"CGB"_sv : mem_->isSGBMode() ? U"SGB"_sv : U"DMG"_sv));
				d.drawLabelAndValue(U"Mapper", U"{}"_fmt(mem_->cartridgeHeader().typeText.substrView(0, 16)));
				d.drawLabelAndValue(U"ROM Bank", U"{:X}"_fmt(mem_->romBank()));
				d.drawLabelAndValue(U"RAM Bank", U"{:X}"_fmt(mem_->ramBank()));
				d.drawEmptyLine();

				// Timer

				d.drawSection(U"Timer");
				d.drawLabelAndValue(U"FF04 DIV ", Uint8ToHexAndBin(mem_->read(Address::DIV)));
				d.drawLabelAndValue(U"FF05 TIMA", Uint8ToHexAndBin(mem_->read(Address::TIMA)));
				d.drawLabelAndValue(U"FF06 TMA ", Uint8ToHexAndBin(mem_->read(Address::TMA)));
				d.drawLabelAndValue(U"FF07 TAC ", Uint8ToHexAndBin(mem_->read(Address::TAC)));
				d.drawEmptyLine();

				// Rendering

				d.drawSection(U"Rendering");
				d.drawLabelAndValue(U"FF40 LCDC", Uint8ToHexAndBin(mem_->read(Address::LCDC)));
				d.drawLabelAndValue(U"FF41 STAT", Uint8ToHexAndBin(mem_->read(Address::STAT)));
				d.drawLabelAndValue(U"FF42 SCY ", Uint8ToHexAndBin(mem_->read(Address::SCY)));
				d.drawLabelAndValue(U"FF43 SCX ", Uint8ToHexAndBin(mem_->read(Address::SCX)));
				d.drawLabelAndValue(U"FF44 LY  ", Uint8ToHexAndBin(mem_->read(Address::LY)));
				d.drawLabelAndValue(U"FF45 LYC ", Uint8ToHexAndBin(mem_->read(Address::LYC)));
				d.drawLabelAndValue(U"FF47 BGP ", Uint8ToHexAndBin(mem_->read(Address::BGP)));
				d.drawLabelAndValue(U"FF48 OBP0", Uint8ToHexAndBin(mem_->read(Address::OBP0)));
				d.drawLabelAndValue(U"FF49 OBP1", Uint8ToHexAndBin(mem_->read(Address::OBP1)));
				d.drawLabelAndValue(U"FF4A WY  ", Uint8ToHexAndBin(mem_->read(Address::WY)));
				d.drawLabelAndValue(U"FF4B WX  ", Uint8ToHexAndBin(mem_->read(Address::WX)));
				d.drawEmptyLine();

				// スペース

				//d.drawEmptyLine();

				// Memory dump

				d.drawSection(U"Memory ({:04X})"_fmt(dumpAddress_));
				d.drawMemoryDump(mem_, dumpAddress_);
				d.drawEmptyLine();
			}

			{
				DrawDebugItem d{ Vec2{ ColumnWidth * 1, 0 } + Padding };

				// CPU

				const auto cpuState = cpu_->getCurrentCPUState();

				d.drawSection(U"CPU");
				d.drawLabelAndValue(U"AF  ", U"{:04X}"_fmt(cpuState.af));
				d.drawLabelAndValue(U"BC  ", U"{:04X}"_fmt(cpuState.bc));
				d.drawLabelAndValue(U"DE  ", U"{:04X}"_fmt(cpuState.de));
				d.drawLabelAndValue(U"HL  ", U"{:04X}"_fmt(cpuState.hl));
				d.drawLabelAndValue(U"SP  ", U"{:04X}"_fmt(cpuState.sp));
				d.drawLabelAndValue(U"PC  ", U"{:04X}"_fmt(cpuState.pc));
				d.drawLabelAndValue(U"Halt", U"{:d}"_fmt(cpuState.halt));
				d.drawLabelAndValue(U"Double Speed", U"{:d}"_fmt(mem_->read(Address::KEY1) >> 7));
				d.drawEmptyLine();

				// Interrupt

				d.drawSection(U"Interrupt");
				d.drawLabelAndValue(U"IME    ", U"{:d}"_fmt(interrupt_->ime()));
				d.drawLabelAndValue(U"FF0F IF", Uint8ToHexAndBin(mem_->read(Address::IF)));
				d.drawLabelAndValue(U"FFFF IE", Uint8ToHexAndBin(mem_->read(Address::IE)));
				d.drawEmptyLine();

				// Sound

				const auto buffer = apu_->getBufferState();

				d.drawSection(U"Sound");
				d.drawLabelAndValue(U"FF26 NR52", Uint8ToHexAndBin(mem_->read(Address::NR52)));
				d.drawText(U"Stream buffer: {:5d} / {:5d}"_fmt(buffer.remain, buffer.max));
				d.drawChannelsAmplitude(apu_);
				d.drawEmptyLine();

				// Joypad

				const auto gamepad = Gamepad(0);

				d.drawSection(U"Joypad");
				d.drawLabelAndValue(U"FF00 JOYP", Uint8ToHexAndBin(mem_->read(Address::JOYP)));
				d.drawText(U"Gamepad: {}"_fmt(gamepad.isConnected() ? U"connected" : U"not found"));
				d.drawEmptyLine();
			}

			{
				DrawDebugItem d{ Vec2{ ColumnWidth * 2, 0 } + Padding };

				d.drawSection(U"Tile data");
				d.drawTileDataTexture(tileDataTexture_);
				d.drawEmptyLine();

				if (mem_->isCGBMode())
				{
					d.drawTileDataTexture(tileDataTextureCGB_);
					d.drawEmptyLine();
				}
			}
		}

		// テキストボックス
		textbox_.drawAt(Scene::CenterF());
	}

	bool DebugMonitor::isVisibleTextbox() const
	{
		return textbox_.isVisible() || (timerTextboxHidden_.isRunning() && timerTextboxHidden_.sF() < 0.1);
	}
}
