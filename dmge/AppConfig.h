#pragma once

namespace dmge
{
	struct AppConfig
	{
		// 読み込むカートリッジのパス
		String cartridgePath;

		// ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> breakpoints;

		// メモリ書き込み時ブレークポイントを設定するアドレス
		// 16進表記、コンマ区切りで複数指定可能
		Array<uint16> breakpointsMemWrite;

		// ブレークポイントを使用する
		bool enableBreakpoint;

		// config.ini のパース結果を返す
		static AppConfig LoadConfig();

		void print();
	};
}
