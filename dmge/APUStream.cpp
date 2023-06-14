#include "APUStream.h"

namespace dmge
{
	APUStream::APUStream()
	{
		// 最大1s分のバッファ
		wave_.resize(44100);
	}

	APUStream::~APUStream()
	{
	}

	void APUStream::getAudio(float* left, float* right, size_t samplesToWrite)
	{
		const int buffer = bufferRemain();
		const int writable = (buffer >= samplesToWrite) ? samplesToWrite : buffer;

		for (const auto i : step(writable))
		{
			*(left++) = wave_[posHead_].left;
			*(right++) = wave_[posHead_].right;

			posHead_ = (posHead_ + 1) % wave_.size();
		}

		for (const auto i : step(samplesToWrite - writable))
		{
			*(left++) = 0;
			*(right++) = 0;
		}
	}

	bool APUStream::hasEnded()
	{
		return false;
	}

	void APUStream::rewind()
	{
	}

	void APUStream::pushSample(float left, float right)
	{
		wave_[posTail_].left = left;
		wave_[posTail_].right = right;

		posTail_ = (posTail_ + 1) % wave_.size();
	}

	int APUStream::bufferRemain() const
	{
		if (posTail_ >= posHead_)
		{
			return posTail_ - posHead_;
		}

		return posTail_ - posHead_ + wave_.size();
	}

	int APUStream::bufferMaxSize() const
	{
		return wave_.size();
	}
}
