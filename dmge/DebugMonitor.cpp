#include "DebugMonitor.h"
#include "Memory.h"
#include "CPU.h"
#include "APU.h"
#include "Address.h"

namespace dmge
{
	void DrawMonitorAddress(uint16 addr, StringView name, uint8 value, double x, double y)
	{
		if (addr != 0)
			FontAsset(U"debug")(U"{:04X} {:4}={:02X}"_fmt(addr, name, value)).draw(10, x, y, Palette::Whitesmoke);
	}

	DebugMonitor::DebugMonitor(Memory* mem, CPU* cpu, APU* apu)
		: mem_{ mem }, cpu_{ cpu }, apu_{ apu }
	{
		textStateDumpAddress_.text = U"0000";
	}

	void DebugMonitor::draw(Point pos)
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

		{
			const ScopedViewport2D viewport{ pos.x, pos.y, Scene::Width() / 2, Scene::Height() };

			Scene::Rect().draw(Color{ 16 });

			// Group 1 : LCD, Various

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

			for (const auto [index, item] : Indexed(group1))
			{
				const auto& addr = item.first;
				const auto& name = item.second;
				DrawMonitorAddress(addr, name, mem_->read(addr), 1, 1 + 11 * index);
			}

			// Group 2 : Audio

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

			for (const auto [index, item] : Indexed(group2))
			{
				const auto& addr = item.first;
				const auto& name = item.second;
				DrawMonitorAddress(addr, name, mem_->read(addr), 1 + 5 * (12 + 2), 1 + 11 * index);
			}

			// CPU Registers

			const auto registerValues = cpu_->getRegisterValues();

			FontAsset(U"debug")(U"AF={:04X}"_fmt(registerValues.af)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 0, Palette::Whitesmoke);
			FontAsset(U"debug")(U"BC={:04X}"_fmt(registerValues.bc)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 1, Palette::Whitesmoke);
			FontAsset(U"debug")(U"DE={:04X}"_fmt(registerValues.de)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 2, Palette::Whitesmoke);
			FontAsset(U"debug")(U"HL={:04X}"_fmt(registerValues.hl)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 3, Palette::Whitesmoke);
			FontAsset(U"debug")(U"SP={:04X}"_fmt(registerValues.sp)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 4, Palette::Whitesmoke);
			FontAsset(U"debug")(U"PC={:04X}"_fmt(registerValues.pc)).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 5, Palette::Whitesmoke);

			// MBC

			FontAsset(U"debug")(U"ROM={:02X}"_fmt(mem_->romBank())).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 7, Palette::Whitesmoke);
			FontAsset(U"debug")(U"RAM={:02X}"_fmt(mem_->ramBank())).draw(10, 1 + 5 * (12 + 2) * 2, 1 + 11 * 8, Palette::Whitesmoke);

			// Memory dump

			for (uint16 row = 0; row < 4u; row++)
			{
				FontAsset(U"debug")(U"{:04X}: {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} | {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}"_fmt(
					dumpAddress_ + 0x10 * row,
					mem_->read(dumpAddress_ + row * 16 + 0),
					mem_->read(dumpAddress_ + row * 16 + 1),
					mem_->read(dumpAddress_ + row * 16 + 2),
					mem_->read(dumpAddress_ + row * 16 + 3),
					mem_->read(dumpAddress_ + row * 16 + 4),
					mem_->read(dumpAddress_ + row * 16 + 5),
					mem_->read(dumpAddress_ + row * 16 + 6),
					mem_->read(dumpAddress_ + row * 16 + 7),
					mem_->read(dumpAddress_ + row * 16 + 8),
					mem_->read(dumpAddress_ + row * 16 + 9),
					mem_->read(dumpAddress_ + row * 16 + 10),
					mem_->read(dumpAddress_ + row * 16 + 11),
					mem_->read(dumpAddress_ + row * 16 + 12),
					mem_->read(dumpAddress_ + row * 16 + 13),
					mem_->read(dumpAddress_ + row * 16 + 14),
					mem_->read(dumpAddress_ + row * 16 + 15)))
					.draw(10, 1, 1 + 11 * (23 + 1) + 11 * row, Palette::Whitesmoke);
			}

			// APU Stream Buffer

			apu_->draw({ 1, 264 + 11 * 5 });
		}

		if (showDumpAddressTextbox_)
		{
			SimpleGUI::TextBoxAt(textStateDumpAddress_, Scene::Rect().bottomCenter().movedBy(0, -48));
		}
	}
	bool DebugMonitor::isVisibleTextbox()
	{
		return showDumpAddressTextbox_;
	}
}
