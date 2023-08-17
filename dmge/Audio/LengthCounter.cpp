#include "../stdafx.h"
#include "LengthCounter.h"
#include "Channel.h"

namespace dmge
{
	LengthCounter::LengthCounter(Channel* channel)
		: channel_{ channel }
	{
	}

	void LengthCounter::step()
	{
		if (enabled_)
		{
			if (lengthTimer_ > 0)
			{
				--lengthTimer_;
			}

			if (lengthTimer_ == 0)
			{
				channel_->setEnable(false);
			}
		}
	}

	void LengthCounter::trigger(int newLengthTimer)
	{
		if (lengthTimer_ == 0)
		{
			lengthTimer_ = newLengthTimer;
		}
	}

	void LengthCounter::setEnable(bool enable)
	{
		// フレームシーケンサの次のステップが長さカウンターをクロックしないものである場合、
		// NRx4への書き込み時に余分な長さクロックが発生します。
		// この場合、長さカウンタが以前は無効で、現在は有効で、長さカウンタがゼロでない場合、
		// 長さカウンタはデクリメントされます。
		// Refer: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior

		if (not enabled_ && enable && extraLengthClockCond_)
		{
			enabled_ = enable;
			step();
			return;
		}

		enabled_ = enable;
	}

	bool LengthCounter::getEnable() const
	{
		return enabled_;
	}

	void LengthCounter::setLengthTimer(int value)
	{
		lengthTimer_ = value;
	}

	void LengthCounter::setExtraLengthClockCond(bool cond)
	{
		extraLengthClockCond_ = cond;
	}
}
