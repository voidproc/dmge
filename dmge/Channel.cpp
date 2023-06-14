#include "Channel.h"
#include "Memory.h"

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
}
