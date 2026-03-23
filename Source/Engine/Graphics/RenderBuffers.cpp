// Copyright (c) Wojciech Figat. All rights reserved.

#include "RenderBuffers.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "RenderTools.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Renderer/Utils/MultiScaler.h"
#include "Engine/Engine/Engine.h"

// How many frames keep cached buffers for temporal or optional effects?
#define LAZY_FRAMES_COUNT 4

RenderBuffers::RenderBuffers(const SpawnParams& params)
    : ScriptingObject(params)
{
    _useNull = GPUDevice::Instance->GetRendererType() == RendererType::Null;
#define CREATE_TEXTURE(name) name = GPUDevice::Instance->CreateTexture(TEXT(#name)); _resources.Add(name)
    CREATE_TEXTURE(DepthBuffer);
    CREATE_TEXTURE(MotionVectors);
    CREATE_TEXTURE(GBuffer0);
    CREATE_TEXTURE(GBuffer1);
    CREATE_TEXTURE(GBuffer2);
    CREATE_TEXTURE(GBuffer3);
#undef CREATE_TEXTURE
}

String RenderBuffers::CustomBuffer::ToString() const
{
    return Name;
}

RenderBuffers::~RenderBuffers()
{
    Release();
    _resources.ClearDelete();
}

void RenderBuffers::ReleaseUnusedMemory()
{
    // Auto release temporal buffer if not used for some time
    const uint64 frameIndex = Engine::FrameCount;
    if (VolumetricFog)
    {
        ASSERT(VolumetricFogHistory);
        if (frameIndex - LastFrameVolumetricFog >= LAZY_FRAMES_COUNT)
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
	if (name && frameIndex - LastFrame##name >= LAZY_FRAMES_COUNT) \
	{ \
		RenderTargetPool::Release(name); \
		name = nullptr; \
		LastFrame##name = 0; \
	}
    UPDATE_LAZY_KEEP_RT(TemporalSSR);
    UPDATE_LAZY_KEEP_RT(TemporalAA);
    UPDATE_LAZY_KEEP_RT(HalfResDepth);
    UPDATE_LAZY_KEEP_RT(HiZ);
    UPDATE_LAZY_KEEP_RT(LuminanceMap);
#undef UPDATE_LAZY_KEEP_RT
    for (int32 i = CustomBuffers.Count() - 1; i >= 0; i--)
    {
        CustomBuffer* e = CustomBuffers[i];
        if (frameIndex - e->LastFrameUsed >= LAZY_FRAMES_COUNT)
        {
            Delete(e);
            CustomBuffers.RemoveAt(i);
        }
    }
}

GPUTexture* RenderBuffers::RequestHalfResDepth(GPUContext* context)
{
    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (LastFrameHalfResDepth == currentFrame)
        return HalfResDepth;
    if (!MultiScaler::Instance()->IsReady())
        return DepthBuffer;

    auto format = GPU_DEPTH_BUFFER_PIXEL_FORMAT;
    auto width = RenderTools::GetResolution(_width, ResolutionMode::Half);
    auto height = RenderTools::GetResolution(_height, ResolutionMode::Half);
    auto tempDesc = GPUTextureDescription::New2D(width, height, format, DepthBuffer->Flags());

    LastFrameHalfResDepth = currentFrame;
    if (HalfResDepth == nullptr)
    {
        // Missing buffer
        HalfResDepth = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(HalfResDepth, "HalfResDepth");
    }
    else if (HalfResDepth->Width() != width || HalfResDepth->Height() != height || HalfResDepth->Format() != format)
    {
        // Wrong size buffer
        RenderTargetPool::Release(HalfResDepth);
        HalfResDepth = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(HalfResDepth, "HalfResDepth");
    }

    // Generate depth
    MultiScaler::Instance()->DownscaleDepth(context, width, height, DepthBuffer, HalfResDepth->View());

    return HalfResDepth;
}

GPUTexture* RenderBuffers::RequestHiZ(GPUContext* context, bool fullRes, int32 mipLevels)
{
    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (LastFrameHiZ == currentFrame)
        return HiZ;
    if (!MultiScaler::Instance()->IsReady())
        return nullptr;
    LastFrameHiZ = currentFrame;

    // Allocate or resize buffer (with full mip-chain)
    auto format = PixelFormat::R32_Float;
    auto width = fullRes ? _width : Math::Max(_width >> 1, 1);
    auto height = fullRes ? _height : Math::Max(_height >> 1, 1);
    auto desc = GPUTextureDescription::New2D(width, height, mipLevels, format, GPUTextureFlags::ShaderResource);
    bool useCompute = false; // TODO: impl Compute Shader for downscaling depth to HiZ with a single dispatch (eg. FidelityFX Single Pass Downsampler)
    if (useCompute)
        desc.Flags |= GPUTextureFlags::UnorderedAccess;
    else
        desc.Flags |= GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews;
    if (HiZ && HiZ->GetDescription() != desc)
    {
        RenderTargetPool::Release(HiZ);
        HiZ = nullptr;
    }
    if (HiZ == nullptr)
    {
        HiZ = RenderTargetPool::Get(desc);
        RENDER_TARGET_POOL_SET_NAME(HiZ, "HiZ");
#if PLATFORM_WEB
        // Hack to fix WebGPU limitation that requires to specify different sampler type manually to load 32-bit float texture
        SetWebGPUTextureViewSampler(HiZ->View(), GPU_WEBGPU_SAMPLER_TYPE_UNFILTERABLE_FLOAT);
#endif
    }

    // Downscale
    MultiScaler::Instance()->BuildHiZ(context, DepthBuffer, HiZ);

    return HiZ;
}

PixelFormat RenderBuffers::GetOutputFormat() const
{
    auto colorFormat = GraphicsSettings::Get()->RenderColorFormat;
    // TODO: fix incorrect alpha leaking into reflections on PS5 with R11G11B10_Float
    if (_useAlpha || PLATFORM_PS5)
    {
        // Promote to format when alpha when needed
        switch (colorFormat)
        {
        case GraphicsSettings::RenderColorFormats::R11G11B10:
            colorFormat = GraphicsSettings::RenderColorFormats::R16G16B16A16;
            break;
        }
    }
    switch (colorFormat)
    {
    case GraphicsSettings::RenderColorFormats::R11G11B10:
        return PixelFormat::R11G11B10_Float;
    case GraphicsSettings::RenderColorFormats::R8G8B8A8:
        return PixelFormat::R8G8B8A8_UNorm;
    case GraphicsSettings::RenderColorFormats::R16G16B16A16:
        return PixelFormat::R16G16B16A16_Float;
    default:
        return PixelFormat::R32G32B32A32_Float;
    }
}

bool RenderBuffers::GetUseAlpha() const
{
    return _useAlpha;
}

void RenderBuffers::SetUseAlpha(bool value)
{
    _useAlpha = value;
}

const RenderBuffers::CustomBuffer* RenderBuffers::FindCustomBuffer(const StringView& name, bool withLinked) const
{
    if (LinkedCustomBuffers && withLinked)
        return LinkedCustomBuffers->FindCustomBuffer(name, withLinked);
    for (const CustomBuffer* e : CustomBuffers)
    {
        if (e->Name == name)
            return e;
    }
    return nullptr;
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
    if ((width == _width && height == _height) || _useNull)
        return false;
    CHECK_RETURN(width > 0 && height > 0, true);

    bool result = false;

    // Depth Buffer
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

    // Cache data
    _width = width;
    _height = height;
    _aspectRatio = static_cast<float>(width) / height;
    _viewport = Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
    LastEyeAdaptationTime = 0;

    // Flush any pool render targets to prevent over-allocating GPU memory when resizing game viewport
    RenderTargetPool::Flush(false, 4);

    return result;
}

void RenderBuffers::Release()
{
    LastEyeAdaptationTime = 0;
    LinkedCustomBuffers = nullptr;

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
    UPDATE_LAZY_KEEP_RT(HiZ);
    UPDATE_LAZY_KEEP_RT(LuminanceMap);
#undef UPDATE_LAZY_KEEP_RT
    CustomBuffers.ClearDelete();
}
