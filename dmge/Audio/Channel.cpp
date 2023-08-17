#include "../stdafx.h"
#include "Channel.h"
#include "../Memory.h"

namespace dmge
{
	bool Channel::getEnable() const
	{
		return enabled_;
	}

	void Channel::setEnable(bool enable)
	{
		enabled_ = enable;
	}

	bool Channel::getDACEnable() const
	{
		return dacEnabled_;
	}

	void Channel::setDACEnable(bool enable)
	{
		dacEnabled_ = enable;
	}
}
