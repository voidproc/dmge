#pragma once

namespace dmge
{
	class Channel
	{
	public:
		bool getEnable() const;

		void setEnable(bool enable);

		bool getDACEnable() const;

		void setDACEnable(bool enable);

	private:
		// Enabled (NR52)
		bool enabled_ = false;

		// DAC State
		bool dacEnabled_ = false;
	};
}
