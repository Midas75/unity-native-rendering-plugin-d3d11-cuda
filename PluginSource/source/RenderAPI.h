#pragma once

#include "Unity/IUnityGraphics.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define tryLog(format,...)\
do{ \
	if (logEnable&&logFile != NULL) { \
		fprintf(logFile,format,__VA_ARGS__); \
		fflush(logFile);\
	} \
} while(0)

struct IUnityInterfaces;
class RenderAPI
{
public:
	virtual ~RenderAPI() { }
	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

	// Is the API using "reversed" (1.0 at near plane, 0.0 at far plane) depth buffer?
	// Reversed Z is used on modern platforms, and improves depth buffer precision.
	virtual bool GetUsesReverseZ() = 0;
	virtual uint8_t* encodeJPEG(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, size_t* length) = 0;
	virtual void clearCache()=0;
	virtual void setLog(bool enable,const char* path);
protected:
	bool logEnable = false;
	const char* logPath = ".\\renderPlugin.log";
	FILE* logFile = NULL;
};


// Create a graphics API implementation instance for the given API type.
RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType);

