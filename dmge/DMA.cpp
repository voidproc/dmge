#include "DMA.h"
#include "Memory.h"

namespace dmge
{
	DMA::DMA(Memory* mem)
		: mem_{ mem }
	{
	}

	void DMA::start(uint8 valueFF46)
	{
		running_ = true;
		srcAddr_ = valueFF46 * 0x100;
		cycleCount_ = -4 - 4 - 1;  // これで oam_dma_timing.gb をパスする（よくわからない）
		offset_ = 0;
	}

	void DMA::update(int cycles)
	{
		if (not running_) return;

		cycleCount_ += cycles;

		while (cycleCount_ >= 4)
		{
			mem_->writeDirect(0xfe00 + offset_, mem_->read(srcAddr_ + offset_));
			cycleCount_ -= 4;
			++offset_;

			if (offset_ > 0x9f)
			{
				running_ = false;
				return;
			}
		}
	}

	bool DMA::running() const
	{
		return running_;
	}
}
