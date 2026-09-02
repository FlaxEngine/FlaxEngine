// Copyright (c) Wojciech Figat. All rights reserved.

#include "RenderBuffers.h"
#include "RenderContext.h"
#include "RenderTools.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Renderer/Utils/MultiScaler.h"
#include "Engine/Renderer/Culling/IOcclusionCulling.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Scripting/Scripting.h"

// How many frames keep cached buffers for temporal or optional effects?
#define LAZY_FRAMES_COUNT 4
bool UnsupportedOcclusionCulling = false;

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
    UPDATE_LAZY_KEEP_RT(HiZ[0]);
    UPDATE_LAZY_KEEP_RT(HiZ[1]);
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

    auto format = DepthBuffer->Format();
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

GPUTexture* RenderBuffers::RequestHiZ(GPUContext* context, bool fullRes, int32 mipLevels, bool closest, bool powerOfTwo)
{
    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    int32 idx = closest ? 0 : 1;
    if (LastFrameHiZ[idx] == currentFrame)
        return HiZ[idx];
    if (!MultiScaler::Instance()->IsReady())
        return nullptr;
    LastFrameHiZ[idx] = currentFrame;

    // Allocate or resize buffer (with full mip-chain)
    auto format = PixelFormat::R32_Float;
    auto width = fullRes ? _width : Math::Max(_width >> 1, 1);
    auto height = fullRes ? _height : Math::Max(_height >> 1, 1);
    if (powerOfTwo)
    {
        width = Math::RoundUpToPowerOf2(width);
        height = Math::RoundUpToPowerOf2(height);
    }
    auto desc = GPUTextureDescription::New2D(width, height, mipLevels, format, GPUTextureFlags::ShaderResource);
    bool useCompute = false; // TODO: impl Compute Shader for downscaling depth to HiZ with a single dispatch (eg. FidelityFX Single Pass Downsampler)
    if (useCompute)
        desc.Flags |= GPUTextureFlags::UnorderedAccess;
    else
        desc.Flags |= GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews;
    if (HiZ[idx] && HiZ[idx]->GetDescription() != desc)
    {
        RenderTargetPool::Release(HiZ[idx]);
        HiZ[idx] = nullptr;
    }
    if (HiZ[idx] == nullptr)
    {
        HiZ[idx] = RenderTargetPool::Get(desc);
        RENDER_TARGET_POOL_SET_NAME(HiZ[idx], "HiZ");
#if PLATFORM_WEB
        // Hack to fix WebGPU limitation that requires to specify different sampler type manually to load 32-bit float texture
        SetWebGPUTextureViewSampler(HiZ[idx]->View(), GPU_WEBGPU_SAMPLER_TYPE_UNFILTERABLE_FLOAT);
#endif
    }

    // Downscale
    MultiScaler::Instance()->BuildHiZ(context, DepthBuffer, HiZ[idx], closest);

    return HiZ[idx];
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
    auto desc = GPUTextureDescription::New2D(width, height, GPU_DEPTH_BUFFER_PIXEL_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil);
    if (!EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(desc.Format).Support, FormatSupport::DepthStencil | FormatSupport::Texture2D))
        desc.Format = desc.Format == PixelFormat::D24_UNorm_S8_UInt ? PixelFormat::D32_Float_S8X24_UInt : PixelFormat::D32_Float;
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

    if (auto* culling = FromInterface(OcclusionCulling))
        Delete(culling);
    OcclusionCulling = nullptr;

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
    UPDATE_LAZY_KEEP_RT(HiZ[0]);
    UPDATE_LAZY_KEEP_RT(HiZ[1]);
    UPDATE_LAZY_KEEP_RT(LuminanceMap);
#undef UPDATE_LAZY_KEEP_RT
    CustomBuffers.ClearDelete();
}

RenderBuffers::ReadOnlyDepthBuffer RenderBuffers::GetReadOnlyDepthBuffer() const
{
    GPUTexture* depthBuffer = DepthBuffer;
    const bool depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
    GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
    GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    return { depthBufferRTV, depthBufferSRV };
}

void RenderBuffers::OnRendering(const RenderContext& renderContext)
{
    // Initialize occlusion culling
    if (UnsupportedOcclusionCulling)
        return;
    bool enableCulling = EnumHasAllFlags(renderContext.View.Flags, ViewFlags::OcclusionCulling) && !renderContext.View.IsCullingDisabled && !renderContext.View.IsSingleFrame;
    const StringAnsi& occlusionCullingTypeName = GraphicsSettings::Get()->OcclusionCulling;
    if (auto* culling = FromInterface(OcclusionCulling))
    {
        // Check if type still matches and effect is active
        if (culling->GetType().Fullname != occlusionCullingTypeName || !enableCulling)
        {
            Delete(culling);
            OcclusionCulling = nullptr;
        }
    }
    if (!OcclusionCulling && occlusionCullingTypeName.HasChars() && enableCulling)
    {
        const ScriptingTypeHandle occlusionCullingType = Scripting::FindScriptingType(occlusionCullingTypeName);
        if (occlusionCullingType && occlusionCullingType.GetType().GetInterface(IOcclusionCulling::TypeInitializer))
        {
            OcclusionCulling = ToInterface<IOcclusionCulling>(NewObject(occlusionCullingType));
            if (!OcclusionCulling->IsSupported())
            {
                UnsupportedOcclusionCulling = true;
                LOG(Error, "Occlusion Culling system '{}' is unsupported", occlusionCullingTypeName.ToString());
                return;
            }

            if (_usedCulling)
            {
                // Reset existing state to use a fresh CullingIds
                _cullingLocker.Lock();
                for (auto& e : Scenes)
                {
                    for (auto& q : e.Value.Geo)
                    {
                        for (auto& geo : q)
                        {
                            geo.CullingId = 0;
                        }
                    }
                    e.Value.CullingIds.Clear();
                }
                _cullingLocker.Unlock();
            }
            else
                _usedCulling = true;
        }
    }
    if (OcclusionCulling)
        OcclusionCulling->BeginFrame(renderContext);
}

void RenderBuffers::OnSceneRendering(SceneRendering* scene)
{
    if (!Scenes.ContainsKey(scene))
    {
        PROFILE_CPU_NAMED("Init Scene");

        // Register scene
        auto& sceneData = Scenes[scene];
        ListenSceneRendering(scene);

        // Put all existing actors into the render buffer geo storage used for LOD transitions, etc.
        for (int32 i = 0; i < SceneRendering::MAX; i++)
        {
            auto& list = scene->Actors[i];
            auto& geo = sceneData.Geo[i];
            geo.Resize(list.Count());
            auto geoPtr = geo.Get();
            for (int32 j = 0; j < list.Count(); j++)
                geoPtr[j] = GeometryDrawState();
        }
    }
}

GeometryDrawState* RenderBuffers::GetGeometryDrawState(SceneRendering* scene, int32 key, const Actor* actor) const
{
    if (auto* sceneData = Scenes.TryGet(scene))
    {
        auto& list = sceneData->Geo[actor->_drawCategory];
        if (list.IsValidIndex(key))
        {
            return list.Get() + key;
        }
    }
    // TODO: what about loose actors drawn from code? dynamically manage their state here?
    return nullptr;
}

bool RenderBuffers::TestOcclusionCulling(const Actor* actor, uint32& cullingId) const
{
    return TestOcclusionCulling(actor->GetSceneRendering(), actor, actor->GetBox(), cullingId);
}

bool RenderBuffers::TestOcclusionCulling(SceneRendering* scene, const Actor* actor, const BoundingBox& objectBounds, uint32& cullingId, const void* object) const
{
    cullingId = 0;
    if (!OcclusionCulling)
        return true;
    bool result = true;
    if (auto* sceneData = Scenes.TryGet(scene))
    {
        // Get stable CullingId
        const Pair<const Actor*, const void*> key(actor, object);
        _cullingLocker.Lock();
        _cullingIdsOwnerTypes.Add(actor->GetTypeHandle());
        sceneData->CullingIds.TryGet(key, cullingId);
        _cullingLocker.Unlock();

        // Cull
        uint32 cullingIdPrev = cullingId;
        result = OcclusionCulling->IsVisible(objectBounds, cullingId);

        // Update CullingId if got changed
        if (cullingIdPrev != cullingId)
        {
            _cullingLocker.Lock();
            sceneData->CullingIds[key] = cullingId;
            _cullingLocker.Unlock();
        }
    }
    return result;
}

void RenderBuffers::OnSceneRenderingAddActor(SceneRendering* scene, int32 key, Actor* a)
{
    // Init geo state of that object
    if (auto* sceneData = Scenes.TryGet(scene))
    {
        auto& list = sceneData->Geo[a->_drawCategory];
        ASSERT(key >= 0);
        if (list.Count() <= key)
            list.Resize(key + 1);
        list.Get()[key] = GeometryDrawState();
    }
}

void RenderBuffers::OnSceneRenderingUpdateActor(SceneRendering* scene, int32 key, Actor* a, const BoundingSphere& prevBounds, UpdateFlags flags)
{
}

void RenderBuffers::OnSceneRenderingRemoveActor(SceneRendering* scene, int32 key, Actor* a)
{
    // Skip actors that don't have nested sub-objects
    if (!_cullingIdsOwnerTypes.Contains(a->GetTypeHandle()))
        return;
    if (auto* sceneData = Scenes.TryGet(scene))
    {
        for (auto it = sceneData->CullingIds.Begin(); it.IsNotEnd(); ++it)
        {
            if (it->Key.First == a)
            {
                sceneData->CullingIds.Remove(it);
            }
        }
    }
}

void RenderBuffers::OnSceneRenderingClear(SceneRendering* scene)
{
    Scenes.Remove(scene);
}
