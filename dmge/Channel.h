#pragma once

namespace dmge
{
	class Channel
	{
	public:
		bool getEnable() const;

		void setEnable(bool enable);

	private:
		// Enabled (NR52)
		bool enabled_ = false;
	};
}
