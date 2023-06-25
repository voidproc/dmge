#include "DebugMonitor.h"
#include "Memory.h"
#include "CPU.h"
#include "Audio/APU.h"
#include "Address.h"

namespace dmge
{
	constexpr Color FontColor{ 220 };
	constexpr Color BgColor{ 32 };
	constexpr Size FontSize{ 5, 10 };
	constexpr int LineHeight = FontSize.y + 1;

	void DrawMemoryAddressValue(uint16 addr, StringView name, uint8 value, double x, double y)
	{
		if (addr != 0)
		{
			FontAsset(U"debug")(U"{:04X} {:4}={:02X}"_fmt(addr, name, value)).draw(10, x, y, FontColor);
		}
	}

	void DrawMemoryDump(Memory* mem, uint16 addr, double x, double y)
	{
		for (uint16 row = 0; row < 4u; row++)
		{
			FontAsset(U"debug")(U"{:04X}: {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} | {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}"_fmt(
				addr + 0x10 * row,
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
				.draw(10, x, y + 11 * row, FontColor);
		}
	}

	DebugMonitor::DebugMonitor(Memory* mem, CPU* cpu, APU* apu)
		: mem_{ mem }, cpu_{ cpu }, apu_{ apu }
	{
		textStateDumpAddress_.text = U"0000";
	}

	void DebugMonitor::update()
	{
		if (KeyM.up())
		{
			showDumpAddressTextbox_ = true;
			textStateDumpAddress_.active = true;
		}

		if (showDumpAddressTextbox_)
		{
			if (KeyEscape.down())
			{
				showDumpAddressTextbox_ = false;
			}

			if (textStateDumpAddress_.textChanged)
			{
				if (const auto addr = ParseIntOpt<uint16>(textStateDumpAddress_.text, 16);
					addr)
				{
					dumpAddress_ = Min<uint16>(*addr, 0xffc0);
				}
			}
		}
	}

	void DebugMonitor::draw(const Point& pos)
	{
		{
			const ScopedViewport2D viewport{ pos.x, pos.y, Scene::Width() / 2, Scene::Height() };

			Scene::Rect().draw(BgColor);

			// Group 1 : LCD, Various
			//   13 Lines
			//   h : (13 * LineHeight)
			//   w : "XXXX YYYY=ZZ" (12 * FontSize.x)

			const Array<std::pair<uint16, StringView>> group1 = {
				{ Address::LCDC, U"LCDC" },
				{ Address::STAT, U"STAT" },
				{ Address::SCY, U"SCY" },
				{ Address::SCX, U"SCX" },
				{ Address::LY, U"LY" },
				{ Address::LYC, U"LYC" },
				{ Address::DMA, U"DMA" },
				{ Address::BGP, U"BGP" },
				{ Address::OBP0, U"OBP0" },
				{ Address::OBP1, U"OBP1" },
				{ Address::WY, U"WY" },
				{ Address::WX, U"WX" },
				{ Address::SVBK, U"SVBK" },
				{ Address::VBK, U"VBK" },
				{ Address::KEY1, U"KEY1" },
				{ Address::JOYP, U"JOYP" },
				{ Address::SB, U"SB" },
				{ Address::SC, U"SC" },
				//{ Address::DIV, U"DIV" },
				{ Address::TIMA, U"TIMA" },
				{ Address::TMA, U"TMA" },
				{ Address::TAC, U"TAC" },
				{ Address::IF, U"IF" },
				{ Address::IE, U"IE" },
			};

			const Rect group1Area{ 2, 1, 12 * FontSize.x, group1.size() * LineHeight };

			for (const auto [index, item] : Indexed(group1))
			{
				const auto& addr = item.first;
				const auto& name = item.second;
				DrawMemoryAddressValue(addr, name, mem_->read(addr), group1Area.x, group1Area.y + index * LineHeight);
			}

			// Group 2 : Audio
			//  11 Lines
			//  h : (11 * LineHeight)
			//  w : "XXXX YYYY=ZZ" (12 * FontSize.x)

			const Array<std::pair<uint16, StringView>> group2 = {
				{ Address::NR10, U"ENT1" },
				{ Address::NR11, U"LEN1" },
				{ Address::NR12, U"ENV1" },
				{ Address::NR13, U"FRQ1" },
				{ Address::NR14, U"KIK1" },
				{ Address::NR21, U"LEN2" },
				{ Address::NR22, U"ENV2" },
				{ Address::NR23, U"FRQ2" },
				{ Address::NR24, U"KIK2" },
				{ Address::NR30, U"ON_3" },
				{ Address::NR31, U"LEN3" },
				{ Address::NR32, U"ENV3" },
				{ Address::NR33, U"FRQ3" },
				{ Address::NR34, U"KIK3" },
				{ Address::NR41, U"LEN4" },
				{ Address::NR42, U"ENV4" },
				{ Address::NR43, U"FRQ4" },
				{ Address::NR44, U"KIK4" },
				{ Address::NR50, U"VOL" },
				{ Address::NR51, U"L/R" },
				{ Address::NR52, U"ON" },
			};

			const Rect group2Area{ group1Area.tr().x + 2 * FontSize.x, group1Area.y, 12 * FontSize.x, group2.size() * LineHeight };

			for (const auto [index, item] : Indexed(group2))
			{
				const auto& addr = item.first;
				const auto& name = item.second;
				DrawMemoryAddressValue(addr, name, mem_->read(addr), group2Area.x, group2Area.y + index * LineHeight);
			}

			// CPU Registers, MBC State
			//   9 Lines
			//   h : (9 * LineHeight)
			//   w : "XX=YYYY", "XXX=YY" (7 * FontSize.x)

			const Rect group3Area{ group2Area.tr().x + 2 * FontSize.x, group1Area.y, 7 * FontSize.x, 9 * LineHeight };

			const auto registerValues = cpu_->getRegisterValues();

			FontAsset(U"debug")(U"AF={:04X}"_fmt(registerValues.af)).draw(10, group3Area.x, group3Area.y + 0 * LineHeight, FontColor);
			FontAsset(U"debug")(U"BC={:04X}"_fmt(registerValues.bc)).draw(10, group3Area.x, group3Area.y + 1 * LineHeight, FontColor);
			FontAsset(U"debug")(U"DE={:04X}"_fmt(registerValues.de)).draw(10, group3Area.x, group3Area.y + 2 * LineHeight, FontColor);
			FontAsset(U"debug")(U"HL={:04X}"_fmt(registerValues.hl)).draw(10, group3Area.x, group3Area.y + 3 * LineHeight, FontColor);
			FontAsset(U"debug")(U"SP={:04X}"_fmt(registerValues.sp)).draw(10, group3Area.x, group3Area.y + 4 * LineHeight, FontColor);
			FontAsset(U"debug")(U"PC={:04X}"_fmt(registerValues.pc)).draw(10, group3Area.x, group3Area.y + 5 * LineHeight, FontColor);

			FontAsset(U"debug")(U"ROM={:02X}"_fmt(mem_->romBank())).draw(10, group3Area.x, group3Area.y + 7 * LineHeight, FontColor);
			FontAsset(U"debug")(U"RAM={:02X}"_fmt(mem_->ramBank())).draw(10, group3Area.x, group3Area.y + 8 * LineHeight, FontColor);

			// Memory dump
			//   "0000: 00 00 00 00 00 00 00 00 | 00 00 00 00 00 00 00 00" => 55 chars
			//   h : (4 * LineHeight)
			//   w : (55 * FontSize.x)

			const Rect memDumpArea{ group1Area.x, group1Area.bl().y + LineHeight, 55 * FontSize.x, 4 * LineHeight };

			DrawMemoryDump(mem_, dumpAddress_, memDumpArea.x, memDumpArea.y);

			// APU
			//   APU Stream Buffer Gauge

			const Size gaugeSize{ 240, 12 };

			const Rect apuArea{ group1Area.x, memDumpArea.bl().y + LineHeight, gaugeSize };

			const auto buffer = apu_->getBufferState();

			if (buffer.remain > 0)
			{
				RectF{ apuArea.pos, SizeF{ 1.0 * gaugeSize.x * buffer.remain / buffer.max, gaugeSize.y } }.draw(Color{ 128 });
			}
			else
			{
				RectF{ apuArea.pos, gaugeSize }.draw(Palette::Red);
			}

			RectF{ apuArea.pos.movedBy(0.5, 0.5), gaugeSize }.drawFrame(1.0, 0.0, Color{ 220 });
			FontAsset(U"debug")(U"{:5} / 44100"_fmt(buffer.remain)).drawAt(apuArea.center(), FontColor);

			// JoyCon
			//   "JOYCONL=L0 D0 U0 R0, JOYCONR=A0 X0 B0 Y0" => 40 chars
			//   h : (1 * LineHeight)
			//   w : (40 * FontSize.x)

			const Rect joyconArea{ apuArea.x, apuArea.bl().y + LineHeight, 40 * FontSize.x, 1 * LineHeight };

			const auto joyconL = JoyConL(0);
			const auto joyconR = JoyConR(0);
			String textL = U"JoyConL=(none), ";
			String textR = U"JoyConR=(none)";

			if (joyconL.isConnected())
			{
				textL = U"JoyConL=L{:d} D{:d} U{:d} R{:d}, "_fmt(
					joyconL.button0.pressed(), joyconL.button1.pressed(), joyconL.button2.pressed(), joyconL.button3.pressed());
			}

			if (joyconR.isConnected())
			{
				textR = U"JoyConR=L{:d} D{:d} U{:d} R{:d}"_fmt(
					joyconR.button0.pressed(), joyconR.button1.pressed(), joyconR.button2.pressed(), joyconR.button3.pressed());
			}

			FontAsset(U"debug")(textL, textR).draw(10, joyconArea.x, joyconArea.y, FontColor);


			// ProController
			//   "ProCon=L0 D0 U0 R0 A0 B0 P0 M0" => 30 chars
			//   h : (1 * LineHeight)
			//   w : (40 * FontSize.x)

			const Rect proconArea{ joyconArea.x, joyconArea.bl().y + LineHeight, 30 * FontSize.x, 1 * LineHeight };

			const auto procon = ProController(0);
			String textProcon = U"ProCon=(none)";

			if (procon.isConnected())
			{
				textProcon = U"ProCon=L{:d} D{:d} U{:d} R{:d} A{:d} B{:d} P{:d} M{:d}"_fmt(
					procon.povLeft.pressed(), procon.povDown.pressed(), procon.povUp.pressed(), procon.povRight.pressed(), procon.buttonA.pressed(), procon.buttonB.pressed(), procon.buttonPlus.pressed(), procon.buttonMinus.pressed());
			}

			FontAsset(U"debug")(textProcon).draw(10, proconArea.x, proconArea.y, FontColor);
		}

		if (showDumpAddressTextbox_)
		{
			SimpleGUI::TextBoxAt(textStateDumpAddress_, Scene::Rect().bottomCenter().movedBy(0, -48));
		}
	}

	bool DebugMonitor::isVisibleTextbox() const
	{
		return showDumpAddressTextbox_;
	}
}
