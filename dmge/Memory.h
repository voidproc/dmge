#pragma once

#include "Cartridge.h"
#include "Address.h"

namespace dmge
{
	class PPU;
	class Timer;
	class Joypad;

	class Memory
	{
	public:
		Memory();

		void init(PPU* ppu, dmge::Timer* timer, dmge::Joypad* joypad);

		void reset();

		void loadCartridge(FilePath cartridgePath);

		void write(uint16 addr, uint8 value);

		void writeDirect(uint16 addr, uint8 value);

		uint8 read(uint16 addr) const;

		uint16 read16(uint16 addr) const;

		int romBank() const;

		template <class F>
		void setWriteHook(F f)
		{
			writeHooks_.push_back(f);
		}

	private:
		PPU* ppu_ = nullptr;
		Timer* timer_ = nullptr;
		Joypad* joypad_ = nullptr;

		// カートリッジの内容
		Array<uint8> rom_;

		// メモリ全体
		Array<uint8> mem_;

		// RAM
		Array<uint8> sram_;

		// カートリッジのヘッダ情報

		String title_;
		CartridgeType cartridgeType_ = CartridgeType::ROM_ONLY;
		int romSizeKB_ = 0;
		int ramSizeKB_ = 0;
		bool ramEnabled_ = false;
		int romBank_ = 1;
		int ramBank_ = 0;
		int bankingMode_ = 0;

		// メモリ書き込み時フック
		Array<std::function<void(uint16, uint8)>> writeHooks_;

		void readTitle_();
		void readCartridgeType_();
		void readROMSize_();
		void readRAMSize_();
	};
}
