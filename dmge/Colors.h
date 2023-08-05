#pragma once

namespace dmge
{
	namespace Colors
	{
		// DMGにおけるピクセルの色番号
		enum class Gray : uint8
		{
			White = 0,
			LightGray = 1,
			DrakGray = 2,
			Black = 3,
		};

		// DMG 用パレットのプリセット

		inline constexpr int PalettePresetsCount = 9;

		inline constexpr std::array<std::array<ColorF, 4>, PalettePresetsCount> PalettePresets = {
			{
				// (0 番は config ファイルで設定できるカスタムカラー)
				{ ColorF{ U"#e8e8e8" }, ColorF{ U"#a0a0a0" }, ColorF{ U"#585858" }, ColorF{ U"#101010" }, },

				// BGB lcd green
				{ ColorF{ U"#e0f8d0" }, ColorF{ U"#88c070" }, ColorF{ U"#346856" }, ColorF{ U"#081820"}, },

				// BGB grey
				{ ColorF{ U"#e8e8e8" }, ColorF{ U"#a0a0a0" }, ColorF{ U"#585858" }, ColorF{ U"#101010" }, },

				// BGB super gameboy (1-A)
				{ ColorF{ U"#ffefce" }, ColorF{ U"#de944a" }, ColorF{ U"#ad2921" }, ColorF{ U"#311852" }, },

				// GBP-NSO
				{ ColorF{ U"#b5c69c" }, ColorF{ U"#8d9c7b" }, ColorF{ U"#637251" }, ColorF{ U"#303820" }, },

				// DMG-NSO
				{ ColorF{ U"#8cad28" }, ColorF{ U"#6c9421" }, ColorF{ U"#426b29" }, ColorF{ U"#214231" }, },

				// SGB 4-D
				{ ColorF{ U"#f8f8b8" }, ColorF{ U"#90c8c8" }, ColorF{ U"#486878" }, ColorF{ U"#082048" }, },

				// SGB 4-E
				{ ColorF{ U"#f8d8a8" }, ColorF{ U"#e0a878" }, ColorF{ U"#785888" }, ColorF{ U"#002030" }, },

				// SGB 4-H
				{ ColorF{ U"#f8f8c8" }, ColorF{ U"#b8c058" }, ColorF{ U"#808840" }, ColorF{ U"#405028" }, },
			}
		};
	}
}
