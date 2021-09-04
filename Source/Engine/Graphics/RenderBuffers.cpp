// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "RenderBuffers.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Renderer/Utils/MultiScaler.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Engine/Engine.h"

RenderBuffers::RenderBuffers(const SpawnParams& params)
    : PersistentScriptingObject(params)
{
#define CREATE_TEXTURE(name) name = GPUDevice::Instance->CreateTexture(TEXT(#name)); _resources.Add(name)
    CREATE_TEXTURE(DepthBuffer);
    CREATE_TEXTURE(MotionVectors);
    CREATE_TEXTURE(GBuffer0);
    CREATE_TEXTURE(GBuffer1);
    CREATE_TEXTURE(GBuffer2);
    CREATE_TEXTURE(GBuffer3);
    CREATE_TEXTURE(RT1_FloatRGB);
    CREATE_TEXTURE(RT2_FloatRGB);
#undef CREATE_TEXTURE
}

RenderBuffers::~RenderBuffers()
{
    Release();
    _resources.ClearDelete();
}

void RenderBuffers::Prepare()
{
    // Auto release temporal buffer if not used for some time
    const uint64 frameIndex = Engine::FrameCount;
    if (VolumetricFog)
    {
        ASSERT(VolumetricFogHistory);

        if (frameIndex - LastFrameVolumetricFog >= 4)
        {
            RenderTargetPool::Release(VolumetricFog);
            VolumetricFog = nullptr;
            RenderTargetPool::Release(VolumetricFogHistory);
            VolumetricFogHistory = nullptr;
            RenderTargetPool::Release(LocalShadowedLightScattering);
            LocalShadowedLightScattering = nullptr;
            LastFrameVolumetricFog = 0;
        }
    }
#define UPDATE_LAZY_KEEP_RT(name) \
	if (name && frameIndex - LastFrame##name >= 4) \
	{ \
		RenderTargetPool::Release(name); \
		name = nullptr; \
		LastFrame##name = 0; \
	}
    UPDATE_LAZY_KEEP_RT(TemporalSSR);
    UPDATE_LAZY_KEEP_RT(TemporalAA);
    UPDATE_LAZY_KEEP_RT(HalfResDepth);
    UPDATE_LAZY_KEEP_RT(LuminanceMap);
#undef UPDATE_LAZY_KEEP_RT
}

GPUTexture* RenderBuffers::RequestHalfResDepth(GPUContext* context)
{
    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (LastFrameHalfResDepth == currentFrame)
        return HalfResDepth;

    const int32 halfDepthWidth = _width / 2;
    const int32 halfDepthHeight = _height / 2;
    const PixelFormat halfDepthFormat = GPU_DEPTH_BUFFER_PIXEL_FORMAT;

    LastFrameHalfResDepth = currentFrame;
    if (HalfResDepth == nullptr)
    {
        // Missing buffer
        auto tempDesc = GPUTextureDescription::New2D(halfDepthWidth, halfDepthHeight, halfDepthFormat);
        tempDesc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil;
        HalfResDepth = RenderTargetPool::Get(tempDesc);
    }
    else if (HalfResDepth->Width() != halfDepthWidth || HalfResDepth->Height() != halfDepthHeight || HalfResDepth->Format() != halfDepthFormat)
    {
        // Wrong size buffer
        RenderTargetPool::Release(HalfResDepth);
        auto tempDesc = GPUTextureDescription::New2D(halfDepthWidth, halfDepthHeight, halfDepthFormat);
        tempDesc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil;
        HalfResDepth = RenderTargetPool::Get(tempDesc);
    }

    // Generate depth
    MultiScaler::Instance()->DownscaleDepth(context, halfDepthWidth, halfDepthHeight, DepthBuffer, HalfResDepth->View());

    return HalfResDepth;
}

uint64 RenderBuffers::GetMemoryUsage() const
{
    uint64 result = 0;
    for (int32 i = 0; i < _resources.Count(); i++)
        result += _resources[i]->GetMemoryUsage();
    return result;
}

bool RenderBuffers::Init(int32 width, int32 height)
{
    // Skip if resolution won't change
    if (width == _width && height == _height)
        return false;
    CHECK_RETURN(width > 0 && height > 0, true);

    bool result = false;

    // Debug Buffer
    GPUTextureDescription desc = GPUTextureDescription::New2D(width, height, GPU_DEPTH_BUFFER_PIXEL_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil);
    if (GPUDevice::Instance->Limits.HasReadOnlyDepth)
        desc.Flags |= GPUTextureFlags::ReadOnlyDepthView;
    result |= DepthBuffer->Init(desc);

    // MotionBlurPass initializes MotionVectors texture if needed (lazy init - not every game needs it)
    MotionVectors->ReleaseGPU();

    // GBuffer 0
    desc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget;
    desc.Format = GBUFFER0_FORMAT;
    desc.DefaultClearColor = Color::Transparent;
    result |= GBuffer0->Init(desc);

    // GBuffer 1
    desc.Format = GBUFFER1_FORMAT;
    desc.DefaultClearColor = Color::Transparent;
    result |= GBuffer1->Init(desc);

    // GBuffer 2
    desc.Format = GBUFFER2_FORMAT;
    desc.DefaultClearColor = Color(1, 0, 0, 0);
    result |= GBuffer2->Init(desc);

    // GBuffer 3
    desc.Format = GBUFFER3_FORMAT;
    desc.DefaultClearColor = Color::Transparent;
    result |= GBuffer3->Init(desc);

    // Helper HDR buffers
    desc.Format = PixelFormat::R11G11B10_Float;
    desc.DefaultClearColor = Color::Transparent;
    result |= RT1_FloatRGB->Init(desc);
    result |= RT2_FloatRGB->Init(desc);

    // Cache data
    _width = width;
    _height = height;
    _aspectRatio = static_cast<float>(width) / height;
    _viewport = Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
    LastEyeAdaptationTime = 0;

    return result;
}

void RenderBuffers::Release()
{
    LastEyeAdaptationTime = 0;

    for (int32 i = 0; i < _resources.Count(); i++)
        _resources[i]->ReleaseGPU();

    RenderTargetPool::Release(VolumetricFog);
    VolumetricFog = nullptr;
    RenderTargetPool::Release(VolumetricFogHistory);
    VolumetricFogHistory = nullptr;
    RenderTargetPool::Release(LocalShadowedLightScattering);
    LocalShadowedLightScattering = nullptr;
    LastFrameVolumetricFog = 0;

#define UPDATE_LAZY_KEEP_RT(name) \
	RenderTargetPool::Release(name); \
	name = nullptr; \
	LastFrame##name = 0
    UPDATE_LAZY_KEEP_RT(TemporalSSR);
    UPDATE_LAZY_KEEP_RT(TemporalAA);
    UPDATE_LAZY_KEEP_RT(HalfResDepth);
    UPDATE_LAZY_KEEP_RT(LuminanceMap);
#undef UPDATE_LAZY_KEEP_RT
}

String RenderBuffers::ToString() const
{
    return TEXT("RenderBuffers");
}
