// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_NULL

#include "GPUDeviceNull.h"
#include "GPUContextNull.h"
#include "GPUAdapterNull.h"
#include "GPUTextureNull.h"
#include "GPUShaderNull.h"
#include "GPUPipelineStateNull.h"
#include "GPUTimerQueryNull.h"
#include "GPUBufferNull.h"
#include "GPUSamplerNull.h"
#include "GPUVertexLayoutNull.h"
#include "GPUSwapChainNull.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Async/GPUTasksManager.h"

GPUDeviceNull::GPUDeviceNull()
    : GPUDevice(RendererType::Null, ShaderProfile::Unknown)
    , _mainContext(nullptr)
    , _adapter(New<GPUAdapterNull>())
{
}

GPUDevice* GPUDeviceNull::Create()
{
    // Create device
    auto device = New<GPUDeviceNull>();
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

GPUDeviceNull::~GPUDeviceNull()
{
    // Ensure to be disposed
    GPUDeviceNull::Dispose();
}

bool GPUDeviceNull::Init()
{
    TotalGraphicsMemory = 0;
    _state = DeviceState::Created;

    // Init device limits
    {
        auto& limits = Limits;
        Platform::MemoryClear(&limits, sizeof(limits));
        limits.MaximumMipLevelsCount = 14;
        limits.MaximumTexture1DSize = 8192;
        limits.MaximumTexture1DArraySize = 512;
        limits.MaximumTexture2DSize = 8192;
        limits.MaximumTexture2DArraySize = 512;
        limits.MaximumTexture3DSize = 2048;
        limits.MaximumTextureCubeSize = 16384;
        limits.MaximumSamplerAnisotropy = 1;
        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
            FeaturesPerFormat[i] = FormatFeatures(static_cast<PixelFormat>(i), MSAALevel::None, FormatSupport::None);
    }

    // Create main context
    _mainContext = New<GPUContextNull>(this);

    _state = DeviceState::Ready;
    return GPUDevice::Init();
}

void GPUDeviceNull::Draw()
{
    DrawBegin();

    auto context = GetMainContext();

    RenderBegin();
    GetTasksManager()->FrameBegin();
    context->FrameBegin();

    // don't render anything

    context->FrameEnd();
    GetTasksManager()->FrameEnd();
    RenderEnd();

    DrawEnd();
}

void GPUDeviceNull::Dispose()
{
    GPUDeviceLock lock(this);

    // Check if has been disposed already
    if (_state == DeviceState::Disposed)
        return;

    // Set current state
    _state = DeviceState::Disposing;

    // Wait for rendering end
    WaitForGPU();

    // Pre dispose
    preDispose();

    // Clear stuff
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(_adapter);

    // Base
    GPUDevice::Dispose();

    // Set current state
    _state = DeviceState::Disposed;
}

GPUContext* GPUDeviceNull::GetMainContext()
{
    return reinterpret_cast<GPUContext*>(_mainContext);
}

GPUAdapter* GPUDeviceNull::GetAdapter() const
{
    return reinterpret_cast<GPUAdapter*>(_adapter);
}

void* GPUDeviceNull::GetNativePtr() const
{
    return nullptr;
}

bool GPUDeviceNull::LoadContent()
{
    // Skip loading resources
    return false;
}

void GPUDeviceNull::WaitForGPU()
{
}

GPUTexture* GPUDeviceNull::CreateTexture(const StringView& name)
{
    return New<GPUTextureNull>();
}

GPUShader* GPUDeviceNull::CreateShader(const StringView& name)
{
    return New<GPUShaderNull>();
}

GPUPipelineState* GPUDeviceNull::CreatePipelineState()
{
    return New<GPUPipelineStateNull>();
}

GPUTimerQuery* GPUDeviceNull::CreateTimerQuery()
{
    return New<GPUTimerQueryNull>();
}

GPUBuffer* GPUDeviceNull::CreateBuffer(const StringView& name)
{
    return New<GPUBufferNull>();
}

GPUSampler* GPUDeviceNull::CreateSampler()
{
    return New<GPUSamplerNull>();
}

GPUVertexLayout* GPUDeviceNull::CreateVertexLayout(const VertexElements& elements, bool explicitOffsets)
{
    return New<GPUVertexLayoutNull>(elements);
}

GPUSwapChain* GPUDeviceNull::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainNull>(window);
}

GPUConstantBuffer* GPUDeviceNull::CreateConstantBuffer(uint32 size, const StringView& name)
{
    return nullptr;
}

GPUDevice* CreateGPUDeviceNull()
{
    return GPUDeviceNull::Create();
}

#endif
