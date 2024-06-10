#include "RenderAPI.h"
#include "PlatformBase.h"

// Direct3D 11 implementation of RenderAPI.

#if SUPPORT_D3D11

#include <assert.h>
#include <d3d11.h>
#include <cuda_runtime.h>
#include <cuda_d3d11_interop.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpujpeg/gpujpeg_encoder.h"
#include "Unity/IUnityGraphicsD3D11.h"

class RenderAPI_D3D11 : public RenderAPI
{
public:
	RenderAPI_D3D11();
	virtual ~RenderAPI_D3D11() {
	}

	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);

	virtual bool GetUsesReverseZ() { return (int)m_Device->GetFeatureLevel() >= (int)D3D_FEATURE_LEVEL_10_0; }
	virtual uint8_t* encodeJPEG(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, size_t* length);
	virtual void clearCache();
private:
	void CreateResources();
	void ReleaseResources();
	void setCudaMemory(size_t length);
	void setD3DTexture2D(ID3D11DeviceContext* ctx, ID3D11Texture2D* d3dtex);
	void registerToCurrentTexture();
private:
	ID3D11Device* m_Device;
	gpujpeg_encoder* encoder;
	gpujpeg_parameters param;
	gpujpeg_image_parameters param_image;

	//reusable data below,assumming the width/height/rowPitch never change
	ID3D11Texture2D* d3dtex_argb32 = nullptr;
	cudaArray* cuArray = nullptr;
	uint8_t* cudaMemory = nullptr;
	size_t currentCudaMemoryLength = 0;
	cudaGraphicsResource* cgr = nullptr;
	uint8_t* jpegData = nullptr;
	size_t jpegDataLength = 0;
};
void RenderAPI_D3D11::registerToCurrentTexture() {
	if (cgr != nullptr) {
		cudaGraphicsUnregisterResource(cgr);
		cgr = nullptr;
	}
	if (d3dtex_argb32 != nullptr) {
		cudaGraphicsD3D11RegisterResource(&cgr, d3dtex_argb32, cudaGraphicsRegisterFlagsNone);
	}
}
void RenderAPI_D3D11::setD3DTexture2D(ID3D11DeviceContext* ctx, ID3D11Texture2D* d3dtex) {
	D3D11_TEXTURE2D_DESC newDesc, currentDesc;
	d3dtex->GetDesc(&newDesc);
	tryLog("newDesc: format:%d arraySize:%d width:%d height:%d miplevels:%d usage:%d sampleCount: %d\n",
		newDesc.Format,
		newDesc.ArraySize,
		newDesc.Width,
		newDesc.Height,
		newDesc.MipLevels,
		newDesc.Usage,
		newDesc.SampleDesc.Count);
	if (d3dtex_argb32 == nullptr) {
		currentDesc = newDesc;
		currentDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_Device->CreateTexture2D(&currentDesc, 0, &d3dtex_argb32);
		registerToCurrentTexture();
	}
	else {
		d3dtex_argb32->GetDesc(&currentDesc);
		if (
			currentDesc.Width != newDesc.Width
			|| currentDesc.Height != newDesc.Height) {
			currentDesc = newDesc;
			currentDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			d3dtex_argb32->Release();// Release is a ref count,so release this and then cudaUnregister is allowed
			HRESULT hr = m_Device->CreateTexture2D(&currentDesc, 0, &d3dtex_argb32);
			registerToCurrentTexture();
			if (d3dtex_argb32 == nullptr) {
				tryLog("CreateTexture2D failed with hr %ld \n", hr);
				return;
			}
		}
	}
	ctx->CopyResource(d3dtex_argb32, d3dtex);
}
void RenderAPI_D3D11::setCudaMemory(size_t length) {
	if (cudaMemory == nullptr) {
		cudaMalloc(&cudaMemory, length);
		currentCudaMemoryLength = length;
		return;
	}
	else if (length != currentCudaMemoryLength) {
		//cudaMemory!=nullptr && length != currentCudaMemoryLength
		cudaFree(cudaMemory);
		cudaMalloc(&cudaMemory, length);
		currentCudaMemoryLength = length;
		return;
	}
	else {
		//cudaMemory!=nullptr&&length==currentCudaMemoryLength
		return;
	}

}
uint8_t* RenderAPI_D3D11::encodeJPEG(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, size_t* length) {
	gpujpeg_encoder_input encoder_input;
	param_image.width = textureWidth;
	param_image.height = textureHeight;
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	do {
		cudaError_t cuda_error;
		setD3DTexture2D(ctx, (ID3D11Texture2D*)textureHandle);
		cuda_error = cudaGraphicsMapResources(1, &cgr, 0);
		if (cuda_error != cudaSuccess) {
			tryLog("cudaGraphicsMapResources failed with cuda_error %s \n", cudaGetErrorName(cuda_error));
			break;
		}
		cuda_error = cudaGraphicsSubResourceGetMappedArray(&cuArray, cgr, 0, 0);
		if (cuda_error != cudaSuccess) {
			tryLog("cudaGraphicsSubResourceGetMappedArray failed with cuda_error %s \n", cudaGetErrorName(cuda_error));
			break;
		}
		setCudaMemory(rowPitch * textureHeight);
		cuda_error = cudaMemcpy2DFromArray(cudaMemory, rowPitch, cuArray, 0, 0, rowPitch, textureHeight, cudaMemcpyDeviceToDevice);
		if (cuda_error != cudaSuccess) {
			tryLog("cudaMemcpy2DFromArray failed with cuda_error %s \n", cudaGetErrorName(cuda_error));
			break;
		}
		cuda_error = cudaGraphicsUnmapResources(1, &cgr, 0);
		if (cuda_error != cudaSuccess) {
			tryLog("cudaGraphicsUnmapResources failed with cuda_error %s \n", cudaGetErrorName(cuda_error));
			break;
		}
		gpujpeg_encoder_input_set_gpu_image(&encoder_input, cudaMemory);
		gpujpeg_encoder_encode(encoder, &param, &param_image, &encoder_input, &jpegData, &jpegDataLength);
	} while (0);

	ctx->Release();
	*length = jpegDataLength;
	return jpegData;
}
RenderAPI* CreateRenderAPI_D3D11()
{
	return new RenderAPI_D3D11();
}

RenderAPI_D3D11::RenderAPI_D3D11()
	: m_Device(NULL)
{
	encoder = gpujpeg_encoder_create(0);
	gpujpeg_set_default_parameters(&param);
	gpujpeg_image_set_default_parameters(&param_image);
	param_image.color_space = GPUJPEG_RGB;
	param_image.pixel_format = GPUJPEG_4444_U8_P0123;
}
void RenderAPI_D3D11::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
	switch (type)
	{
	case kUnityGfxDeviceEventInitialize:
	{
		IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
		m_Device = d3d->GetDevice();
		CreateResources();
		break;
	}
	case kUnityGfxDeviceEventShutdown:
		ReleaseResources();
		break;
	}
}


void RenderAPI_D3D11::CreateResources()
{

}
void RenderAPI_D3D11::clearCache() {
	if (cgr != nullptr)
	{
		cudaGraphicsUnregisterResource(cgr);
		cgr = nullptr;
		cuArray = nullptr;
	}
	if (cudaMemory != nullptr) {
		cudaFree(cudaMemory);
		cudaMemory = nullptr;
		currentCudaMemoryLength = 0;
	}
	if (d3dtex_argb32 != nullptr)
	{
		d3dtex_argb32->Release();
		d3dtex_argb32 = nullptr;
	}
	gpujpeg_encoder_destroy(encoder);
}

void RenderAPI_D3D11::ReleaseResources()
{
}
#endif // #if SUPPORT_D3D11
