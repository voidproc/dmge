#pragma once

namespace dmge
{
	namespace Address
	{
		enum : uint16
		{
			// ROM Bank 0

			ROMBank0 = 0x0000,
			ROMBank0_End = 0x3fff,

			// Interrupt

			INT40_VBlank = 0x0040,
			INT48_STAT = 0x0048,
			INT50_Timer = 0x0050,
			INT58_Serial = 0x0058,
			INT60_Joypad = 0x0060,

			// Cartridge Header

			EntryPoint = 0x0100,
			EntryPoint_End = 0x103,

			NintendoLogo = 0x0104,
			NintendoLogo_End = 0x0133,

			Title = 0x0134,
			Title_End = 0x0143,

			Manufacturer = 0x013f,
			Manufacturer_End = 0x0142,

			CGBFlag = 0x0143,

			NewLicenseeCode = 0x0144,
			NewLicenseeCode_End = 0x0145,

			SGBFlag = 0x0146,

			CartridgeType = 0x0147,

			ROMSize = 0x0148,

			RAMSize = 0x0149,

			DestinationCode = 0x014a,

			OldLicenseeCode = 0x014b,

			ROMVersionNumber = 0x014c,

			HeaderChecksum = 0x014d,

			GlobalChecksum = 0x014e,
			GlobalChecksum_End = 0x014f,

			// MBC

			MBC_RAMEnable = 0x0000,
			MBC_RAMEnable_End = 0x1fff,

			MBC_ROMBank = 0x2000,
			MBC_ROMBank_End = 0x3fff,

			MBC_ROMBankLow = 0x2000,
			MBC_ROMBankLow_End = 0x2fff,
			MBC_ROMBankHigh = 0x3000,
			MBC_ROMBankHigh_End = 0x3fff,

			MBC_RAMBank = 0x4000,
			MBC_RAMBank_End = 0x5fff,

			MBC_BankingMode = 0x6000,
			MBC_BankingMode_End = 0x7fff,

			MBC_LatchClock = 0x6000,
			MBC_LatchClock_End = 0x7fff,

			// Switchable ROM Bank

			SwitchableROMBank = 0x4000,
			SwitchableROMBank_End = 0x7fff,

			// VRAM

			VRAM = 0x8000,
			VRAM_End = 0x9fff,

			// VRAM Tile Data

			TileData0 = 0x8000,
			TileData0_End = 0x87ff,

			TileData1 = 0x8800,
			TileData1_End = 0x8fff,

			TileData2 = 0x9000,
			TileData2_End = 0x97ff,

			// VRAM Tile Map

			TileMap0 = 0x9800,
			TileMap0_End = 0x9bff,

			TileMap1 = 0x9c00,
			TileMap1_End = 0x9fff,

			// External RAM (SRAM)

			SRAM = 0xa000,
			SRAM_End = 0xbfff,

			// WRAM

			WRAM = 0xc000,
			WRAM_End = 0xdfff,

			WRAM0 = 0xc000,
			WRAM0_End = 0xcfff,

			WRAM1 = 0xd000,
			WRAM1_End = 0xdfff,

			// Echo of WRAM (c000-ddff)

			EchoRAM = 0xe000,
			EchoRAM_End = 0xfdff,

			// Sprite attribute table (OAM)

			OAMTable = 0xfe00,
			OAMTable_End = 0xfe9f,

			// I/O Registers

			IORegisters = 0xff00,

			JOYP = 0xff00,
			SB = 0xff01,
			SC = 0xff02,
			DIV = 0xff04,
			TIMA = 0xff05,
			TMA = 0xff06,
			TAC = 0xff07,
			IF = 0xff0f,
			NR10 = 0xff10,
			NR11 = 0xff11,
			NR12 = 0xff12,
			NR13 = 0xff13,
			NR14 = 0xff14,
			NR21 = 0xff16,
			NR22 = 0xff17,
			NR23 = 0xff18,
			NR24 = 0xff19,
			NR30 = 0xff1a,
			NR31 = 0xff1b,
			NR32 = 0xff1c,
			NR33 = 0xff1d,
			NR34 = 0xff1e,
			NR41 = 0xff20,
			NR42 = 0xff21,
			NR43 = 0xff22,
			NR44 = 0xff23,
			NR50 = 0xff24,
			NR51 = 0xff25,
			NR52 = 0xff26,
			WaveRAM = 0xff30,
			LCDC = 0xff40,
			STAT = 0xff41,
			SCY = 0xff42,
			SCX = 0xff43,
			LY = 0xff44,
			LYC = 0xff45,
			DMA = 0xff46,
			BGP = 0xff47,
			OBP0 = 0xff48,
			OBP1 = 0xff49,
			WY = 0xff4a,
			WX = 0xff4b,
			KEY1 = 0xff4d,
			VBK = 0xff4f,
			BANK = 0xff50,  // Enable Boot ROM
			HDMA1 = 0xff51,
			HDMA2 = 0xff52,
			HDMA3 = 0xff53,
			HDMA4 = 0xff54,
			HDMA5 = 0xff55,
			RP = 0xff56,
			BCPS = 0xff68,
			BGPI = 0xff68,
			BCPD = 0xff69,
			BGPD = 0xff69,
			OCPS = 0xff6a,
			OBPI = 0xff6a,
			OCPD = 0xff6b,
			OBPD = 0xff6b,
			OPRI = 0xff6c,
			SVBK = 0xff70,
			PCM12 = 0xff76,
			PCM34 = 0xff77,

			IORegisters_End = 0xff7f,

			// HRAM

			HRAM = 0xff80,
			HRAM_End = 0xfffe,

			// IE
			IE = 0xffff,
		};
	}
}

#define ADDRESS_IN_RANGE(addr,name) (name<=addr&&addr<=name##_End)
