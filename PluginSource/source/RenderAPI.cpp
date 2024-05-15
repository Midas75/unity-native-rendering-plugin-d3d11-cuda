#include "RenderAPI.h"
#include "PlatformBase.h"
#include "Unity/IUnityGraphics.h"

RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType)
{
#	if SUPPORT_D3D11
	if (apiType == kUnityGfxRendererD3D11)
	{
		extern RenderAPI* CreateRenderAPI_D3D11();
		return CreateRenderAPI_D3D11();
	}
#	endif // if SUPPORT_D3D11
	// Unknown or unsupported graphics API
	return NULL;
}
void RenderAPI::setLog(bool enable, const char* path) {
	if (logEnable&&logFile!= NULL) {
		fclose(logFile);
		logFile = NULL;
	}
	logEnable = enable;
	logPath = path;
	if (logEnable) {
		logFile = fopen(path, "a");
	}
	if (!logEnable&&logFile!= NULL) {
		fclose(logFile);
	}
}