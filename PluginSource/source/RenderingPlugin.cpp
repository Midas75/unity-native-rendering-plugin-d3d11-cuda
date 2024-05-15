#include "PlatformBase.h"
#include "RenderAPI.h"

#include <assert.h>
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces * unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}
static RenderAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		assert(s_CurrentAPI == NULL);
		s_DeviceType = s_Graphics->GetRenderer();
		s_CurrentAPI = CreateRenderAPI(s_DeviceType);
	}
	if (s_CurrentAPI)
	{
		s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
	}
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		delete s_CurrentAPI;
		s_CurrentAPI = NULL;
		s_DeviceType = kUnityGfxRendererNull;
	}
}
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API SetLog(bool enable,const char * path) {
	s_CurrentAPI->setLog(enable,path);
	return;
}
extern "C" UNITY_INTERFACE_EXPORT  uint8_t * UNITY_INTERFACE_API EncodeJPEG(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, size_t * length)
{
	return s_CurrentAPI->encodeJPEG(textureHandle, textureWidth, textureHeight, rowPitch, length);
}
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API ClearCache() {
	s_CurrentAPI->clearCache();
	return;
}
