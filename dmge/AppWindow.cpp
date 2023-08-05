#include "AppWindow.h"
#include "DebugMonitor.h"

namespace dmge
{
	void SetScaleWindowSize(int scale, bool showDebugMonitor, Centering centering)
	{
		const int width = 160 * scale;
		const int height = 144 * scale;

		const int widthWithDebugMonitor = width + dmge::DebugMonitor::ViewportSize.x;
		const int heightWithDebugMonitor = Max(height, dmge::DebugMonitor::ViewportSize.y);

		const auto SceneSize = showDebugMonitor ? Size{ widthWithDebugMonitor, heightWithDebugMonitor } : Size{ width, height };
		Scene::Resize(SceneSize);
		Window::Resize(SceneSize, centering);
	}
}
