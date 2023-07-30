#pragma once

namespace dmge
{
	namespace SGB
	{
		// SGB System Commands
		enum class Commands : uint8
		{
			PAL01 = 0x00,
			PAL23 = 0x01,
			PAL03 = 0x02,
			PAL12 = 0x03,
			PAL_SET = 0x0a,
			PAL_TRN = 0x0b,
			PAL_PRI = 0x19,

			ATTR_BLK = 0x04,
			ATTR_LIN = 0x05,
			ATTR_DIV = 0x06,
			ATTR_CHR = 0x07,
			ATTR_TRN = 0x15,
			ATTR_SET = 0x16,

			SOUND = 0x08,
			SOU_TRN = 0x09,

			MASK_EN = 0x17,
			ATRC_EN = 0x0c,
			TEST_EN = 0x0d,
			ICON_EN = 0x0e,
			DATA_SND = 0x0f,
			DATA_TRN = 0x10,
			JUMP = 0x12,

			MLT_REQ = 0x11,

			CHR_TRN = 0x13,
			PCT_TRN = 0x14,
			OBJ_TRN = 0x18,
		};
	}
}
