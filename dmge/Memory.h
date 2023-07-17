#pragma once

#include "Cartridge.h"
#include "Address.h"
#include "DMA.h"

namespace dmge
{
	class MBC;
	class PPU;
	class APU;
	class Timer;
	class Joypad;
	class Serial;
	class LCD;
	class Interrupt;

	class Memory
	{
	public:
		Memory();

		~Memory();

		void init(PPU* ppu, APU* apu, dmge::Timer* timer, dmge::Joypad* joypad, LCD* lcd, Interrupt* interrupt, Serial* serial);

		void reset();

		bool loadCartridge(FilePath cartridgePath);

		void loadSRAM();

		void saveSRAM();

		void write(uint16 addr, uint8 value);

		void writeDirect(uint16 addr, uint8 value);

		uint8 read(uint16 addr) const;

		uint8 readVRAMBank(uint16 addr, int bank) const;

		uint16 read16(uint16 addr) const;

		uint16 read16VRAMBank(uint16 addr, int bank) const;

		void update(int cycles);

		bool isCGBMode() const;

		int romBank() const;

		int ramBank() const;

		int vramBank() const;

		void dumpCartridgeInfo();

		void dump(uint16 addrBegin, uint16 addrEnd);

		void enableBootROM(FilePathView bootROMPath);

		void disableBootROM();

		template <class F>
		void setWriteHook(F f)
		{
			writeHooks_.push_back(f);
		}

		bool isVRAMTileDataModified();
		void resetVRAMTileDataModified();

		void setDoubleSpeed(bool enable);
		bool isDoubleSpeed() const;

	private:
		PPU* ppu_ = nullptr;
		APU* apu_ = nullptr;
		Timer* timer_ = nullptr;
		Joypad* joypad_ = nullptr;
		Serial* serial_ = nullptr;
		LCD* lcd_ = nullptr;
		Interrupt* interrupt_ = nullptr;

		// MBC
		std::unique_ptr<MBC> mbc_;

		// メモリ全体
		Array<uint8> mem_{};

		// VRAM
		std::array<std::array<uint8, 0x2000>, 2> vram_{};
		int vramBank_ = 0;
		bool vramTileDataModified_ = false;

		// WRAM
		std::array<std::array<uint8, 0x1000>, 8> wram_{};
		int wramBank_ = 1;

		// OAM DMA
		DMA dma_{ this };

		// メモリ書き込み時フック
		Array<std::function<void(uint16, uint8)>> writeHooks_{};

		// (CGB)倍速モード
		bool doubleSpeed_ = false;
		bool doubleSpeedPrepared_ = false;
	};
}
