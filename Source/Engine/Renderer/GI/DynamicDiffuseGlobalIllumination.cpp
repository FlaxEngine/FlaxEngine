// Copyright (c) Wojciech Figat. All rights reserved.

#include "DynamicDiffuseGlobalIllumination.h"
#include "GlobalSurfaceAtlasPass.h"
#include "../GlobalSignDistanceFieldPass.h"
#include "../RenderList.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Level/Actors/BrushMode.h"
#include "Engine/Renderer/GBufferPass.h"

// Implementation based on:
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Probes", Journal of Computer Graphics Tools, April 2019
// Zander Majercik, Jean-Philippe Guertin, Derek Nowrouzezahrai, and Morgan McGuire
// https://morgan3d.github.io/articles/2019-04-01-ddgi/index.html and https://gdcvault.com/play/1026182/
//
// Additional references:
// "Scaling Probe-Based Real-Time Dynamic Global Illumination for Production", https://jcgt.org/published/0010/02/01/
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields", https://jcgt.org/published/0008/02/01/

// This must match HLSL
#define DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT 4096 // Maximum amount of probes to update at once during rays tracing and blending
#define DDGI_TRACE_RAYS_LIMIT 256 // Limit of rays per-probe (runtime value can be smaller)
#define DDGI_PROBE_RESOLUTION_IRRADIANCE 6 // Resolution (in texels) for probe irradiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_DISTANCE 14 // Resolution (in texels) for probe distance data (excluding 1px padding on each side)
#define DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE 8
#define DDGI_PROBE_CLASSIFY_GROUP_SIZE 32
#define DDGI_DEBUG_STATS 0 // Enables additional GPU-driven stats for probe/rays count
#define DDGI_DEBUG_INSTABILITY 0 // Enables additional probe irradiance instability debugging

#if DDGI_DEBUG_STATS
#include "Engine/Core/Collections/SamplesBuffer.h"
#define DDGI_DEBUG_STATS_FRAMES 60

struct StatsData
{
    uint32 RaysCount;
    uint32 ProbesCount;
};
#endif

GPU_CB_STRUCT(Data0 {
    DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
    GlobalSurfaceAtlasPass::ConstantsData GlobalSurfaceAtlas;
    ShaderGBufferData GBuffer;
    Float4 RaysRotation;
    float SkyboxIntensity;
    uint32 ProbesCount;
    float ResetBlend;
    float TemporalTime;
    Int4 ProbeScrollClears[4];
    Float3 ViewDir;
    float Padding1;
    });

GPU_CB_STRUCT(Data1 {
    // TODO: use push constants on Vulkan or root signature data on DX12 to reduce overhead of changing single DWORD
    Float2 Padding2;
    uint32 CascadeIndex;
    uint32 ProbeIndexOffset;
    });

class DDGICustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    struct
    {
        Float3 ProbesOrigin;
        float ProbesSpacing = 0.0f;
        Int3 ProbeScrollOffsets;
        Int3 ProbeScrollClears;

        void Clear()
        {
            ProbesOrigin = Float3::Zero;
            ProbeScrollOffsets = Int3::Zero;
            ProbeScrollClears = Int3::Zero;
        }
    } Cascades[4];

    int32 CascadesCount = 0;
    int32 ProbeRaysCount = 0;
    int32 ProbesCountTotal = 0;
    Int3 ProbeCounts = Int3::Zero;
    GPUTexture* ProbesTrace = nullptr; // Probes ray tracing: (RGB: hit radiance, A: hit distance)
    GPUTexture* ProbesData = nullptr; // Probes data: (RGB: probe-space offset, A: state/data)
    GPUTexture* ProbesIrradiance = nullptr; // Probes irradiance (RGB: sRGB color)
    GPUTexture* ProbesDistance = nullptr; // Probes distance (R: mean distance, G: mean distance^2)
    GPUBuffer* ActiveProbes = nullptr; // List with indices of the active probes (built during probes classification to use indirect dispatches for probes updating), counter at 0
    GPUBuffer* UpdateProbesInitArgs = nullptr; // Indirect dispatch buffer for active-only probes updating (trace+blend)
#if DDGI_DEBUG_STATS
    GPUBuffer* StatsWrite = nullptr;
    GPUBuffer* StatsRead = nullptr;
    SamplesBuffer<uint32, DDGI_DEBUG_STATS_FRAMES> StatsProbes;
    SamplesBuffer<uint32, DDGI_DEBUG_STATS_FRAMES> StatsRays;
    uint32 StatsFrames = 0;
#endif
#if DDGI_DEBUG_INSTABILITY
    GPUTexture* ProbesInstability = nullptr;
#endif
    DynamicDiffuseGlobalIlluminationPass::BindingData Result;

    FORCE_INLINE void Release()
    {
        RenderTargetPool::Release(ProbesTrace);
        RenderTargetPool::Release(ProbesData);
        RenderTargetPool::Release(ProbesIrradiance);
        RenderTargetPool::Release(ProbesDistance);
        SAFE_DELETE_GPU_RESOURCE(ActiveProbes);
        SAFE_DELETE_GPU_RESOURCE(UpdateProbesInitArgs);
#if DDGI_DEBUG_STATS
        SAFE_DELETE_GPU_RESOURCE(StatsWrite);
        SAFE_DELETE_GPU_RESOURCE(StatsRead);
        StatsProbes.Clear();
        StatsRays.Clear();
        StatsFrames = 0;
#endif
#if DDGI_DEBUG_INSTABILITY
        RenderTargetPool::Release(ProbesInstability);
#endif
    }

    ~DDGICustomBuffer()
    {
        Release();
    }
};

void CalculateVolumeRandomRotation(Matrix3x3& matrix)
{
    // Reference: James Arvo's algorithm Graphics Gems 3 (pages 117-120)
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.1357&rep=rep1&type=pdf

    float u1 = TWO_PI * Random::Rand();
    float cos1 = Math::Cos(u1);
    float sin1 = Math::Sin(u1);
    float u2 = TWO_PI * Random::Rand();
    float cos2 = Math::Cos(u2);
    float sin2 = Math::Sin(u2);

    float u3 = Random::Rand();
    float sq3 = 2.0f * sqrtf(u3 * (1.0f - u3));

    float s2 = 2.0f * u3 * sin2 * sin2 - 1.0f;
    float c2 = 2.0f * u3 * cos2 * cos2 - 1.0f;
    float sc = 2.0f * u3 * sin2 * cos2;

    matrix.M11 = cos1 * c2 - sin1 * sc;
    matrix.M12 = sin1 * c2 + cos1 * sc;
    matrix.M13 = sq3 * cos2;

    matrix.M21 = cos1 * sc - sin1 * s2;
    matrix.M22 = sin1 * sc + cos1 * s2;
    matrix.M23 = sq3 * sin2;

    matrix.M31 = cos1 * (sq3 * cos2) - sin1 * (sq3 * sin2);
    matrix.M32 = sin1 * (sq3 * cos2) + cos1 * (sq3 * sin2);
    matrix.M33 = 1.0f - 2.0f * u3;
}

String DynamicDiffuseGlobalIlluminationPass::ToString() const
{
    return TEXT("DynamicDiffuseGlobalIlluminationPass");
}

bool DynamicDiffuseGlobalIlluminationPass::Init()
{
    // Check platform support
    const auto device = GPUDevice::Instance;
    _supported = device->GetFeatureLevel() >= FeatureLevel::SM5 && device->Limits.HasCompute && device->Limits.HasTypedUAVLoad;
    return false;
}

bool DynamicDiffuseGlobalIlluminationPass::setupResources()
{
    if (!_supported)
        return true;

    // Load shader
    if (!_shader)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GI/DDGI"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<DynamicDiffuseGlobalIlluminationPass, &DynamicDiffuseGlobalIlluminationPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;

    // Initialize resources
    const auto shader = _shader->GetShader();
    _cb0 = shader->GetCB(0);
    _cb1 = shader->GetCB(1);
    if (!_cb0 || !_cb1)
        return true;
    _csClassify = shader->GetCS("CS_Classify");
    _csUpdateProbesInitArgs = shader->GetCS("CS_UpdateProbesInitArgs");
    _csTraceRays[0] = shader->GetCS("CS_TraceRays", 0);
    _csTraceRays[1] = shader->GetCS("CS_TraceRays", 1);
    _csTraceRays[2] = shader->GetCS("CS_TraceRays", 2);
    _csTraceRays[3] = shader->GetCS("CS_TraceRays", 3);
    _csUpdateProbesIrradiance = shader->GetCS("CS_UpdateProbes", 0);
    _csUpdateProbesDistance = shader->GetCS("CS_UpdateProbes", 1);
    auto device = GPUDevice::Instance;
    auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psIndirectLighting[0])
    {
        _psIndirectLighting[0] = device->CreatePipelineState();
        _psIndirectLighting[1] = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_IndirectLighting");
        psDesc.BlendMode = BlendingMode::Add;
        if (_psIndirectLighting[0]->Init(psDesc))
            return true;
        psDesc.PS = shader->GetPS("PS_IndirectLighting", 1);
        if (_psIndirectLighting[1]->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void DynamicDiffuseGlobalIlluminationPass::OnShaderReloading(Asset* obj)
{
    LastFrameShaderReload = Engine::FrameCount;
    _csClassify = nullptr;
    _csUpdateProbesInitArgs = nullptr;
    _csTraceRays[0] = nullptr;
    _csTraceRays[1] = nullptr;
    _csTraceRays[2] = nullptr;
    _csTraceRays[3] = nullptr;
    _csUpdateProbesIrradiance = nullptr;
    _csUpdateProbesDistance = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting[0]);
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting[1]);
    invalidateResources();
}

#endif

void DynamicDiffuseGlobalIlluminationPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    _cb0 = nullptr;
    _cb1 = nullptr;
    _shader = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting[0]);
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting[1]);
#if USE_EDITOR
    _debugModel = nullptr;
    _debugMaterial = nullptr;
#endif
}

bool DynamicDiffuseGlobalIlluminationPass::Get(const RenderBuffers* buffers, BindingData& result)
{
    auto* ddgiData = buffers ? buffers->FindCustomBuffer<DDGICustomBuffer>(TEXT("DDGI")) : nullptr;
    if (ddgiData && ddgiData->LastFrameUsed + 1 >= Engine::FrameCount) // Allow to use data from the previous frame (eg. particles in Editor using the Editor viewport in Game viewport - Game render task runs first)
    {
        result = ddgiData->Result;
        return false;
    }
    return true;
}

bool DynamicDiffuseGlobalIlluminationPass::RenderInner(RenderContext& renderContext, GPUContext* context, DDGICustomBuffer& ddgiData)
{
    // Render Global SDF and Global Surface Atlas for software raytracing
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF))
        return true;
    GlobalSurfaceAtlasPass::BindingData bindingDataSurfaceAtlas;
    if (GlobalSurfaceAtlasPass::Instance()->Render(renderContext, context, bindingDataSurfaceAtlas))
        return true;
    GPUTextureView* skybox = GBufferPass::Instance()->RenderSkybox(renderContext, context);

    // Setup options
    auto& settings = renderContext.List->Settings.GlobalIllumination;
    auto* graphicsSettings = GraphicsSettings::Get();
    const float probesSpacing = Math::Clamp(graphicsSettings->GIProbesSpacing, 10.0f, 1000.0f); // GI probes placement spacing nearby camera (for closest cascade; gets automatically reduced for further cascades)
    int32 probeRaysCount; // Amount of rays to trace randomly around each probe
    switch (Graphics::GIQuality) // Ensure to match CS_TraceRays permutations
    {
    case Quality::Low:
        probeRaysCount = 96;
        break;
    case Quality::Medium:
        probeRaysCount = 128;
        break;
    case Quality::High:
        probeRaysCount = 192;
        break;
    case Quality::Ultra:
        probeRaysCount = 256;
        break;
    default:
        return true;
    }
    ASSERT_LOW_LAYER(probeRaysCount <= DDGI_TRACE_RAYS_LIMIT);
    const float indirectLightingIntensity = settings.Intensity;
    const float probeHistoryWeight = Math::Clamp(settings.TemporalResponse, 0.0f, 0.98f);
    const float distance = settings.Distance;
    const Color fallbackIrradiance = settings.FallbackIrradiance;

    // Automatically calculate amount of cascades to cover the GI distance at the current probes spacing
    const int32 idealProbesCount = 20; // Ideal amount of probes per-cascade to try to fit in order to cover whole distance
    int32 cascadesCount = 1;
    float idealDistance = idealProbesCount * probesSpacing;
    while (cascadesCount < 4 && idealDistance < distance)
    {
        idealDistance *= 2;
        cascadesCount++;
    }

    // Calculate the probes count based on the amount of cascades and the distance to cover
    const float cascadesDistanceScales[] = { 1.0f, 3.0f, 6.0f, 10.0f }; // Scales each cascade further away from the camera origin
    const float distanceExtent = distance / cascadesDistanceScales[cascadesCount - 1];
    const float verticalRangeScale = 0.8f; // Scales the probes volume size at Y axis (horizontal aspect ratio makes the DDGI use less probes vertically to cover whole screen)
    Int3 probesCounts(Float3::Ceil(Float3(distanceExtent, distanceExtent * verticalRangeScale, distanceExtent) / probesSpacing));
    const int32 maxProbeSize = Math::Max(DDGI_PROBE_RESOLUTION_IRRADIANCE, DDGI_PROBE_RESOLUTION_DISTANCE) + 2;
    const int32 maxTextureSize = Math::Min(GPUDevice::Instance->Limits.MaximumTexture2DSize, GPU_MAX_TEXTURE_SIZE);
    while (probesCounts.X * probesCounts.Y * maxProbeSize > maxTextureSize
        || probesCounts.Z * cascadesCount * maxProbeSize > maxTextureSize)
    {
        // Decrease quality to ensure the probes texture won't overflow
        probesCounts -= 1;
    }

    // Initialize cascades
    float probesSpacings[4];
    Float3 viewOrigins[4];
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        // Each cascade has higher spacing between probes
        float cascadeDistanceScale = cascadesDistanceScales[cascadeIndex];
        float cascadeProbesSpacing = probesSpacing * cascadeDistanceScale;
        probesSpacings[cascadeIndex] = cascadeProbesSpacing;

        // Calculate view origin for cascade by shifting it towards the view direction to account for better view frustum coverage
        Float3 viewOrigin = renderContext.View.Position;
        Float3 viewDirection = renderContext.View.Direction;
        const Float3 probesDistance = Float3(probesCounts) * cascadeProbesSpacing;
        const float probesDistanceMax = probesDistance.MaxValue();
        const Float3 viewRayHit = CollisionsHelper::LineHitsBox(viewOrigin, viewOrigin + viewDirection * (probesDistanceMax * 2.0f), viewOrigin - probesDistance, viewOrigin + probesDistance);
        const float viewOriginOffset = viewRayHit.Y * probesDistanceMax * 0.6f;
        viewOrigin += viewDirection * viewOriginOffset;
        const float viewOriginSnapping = cascadeProbesSpacing;
        viewOrigin = Float3::Floor(viewOrigin / viewOriginSnapping) * viewOriginSnapping;
        //viewOrigin = Float3::Zero;
        viewOrigins[cascadeIndex] = viewOrigin;
    }

    // Init buffers
    const int32 probesCountCascade = probesCounts.X * probesCounts.Y * probesCounts.Z;
    const int32 probesCountTotal = probesCountCascade * cascadesCount;
    if (probesCountTotal == 0 || indirectLightingIntensity <= ZeroTolerance)
        return true;
    int32 probesCountCascadeX = probesCounts.X * probesCounts.Y;
    int32 probesCountCascadeY = probesCounts.Z;
    int32 probesCountTotalX = probesCountCascadeX;
    int32 probesCountTotalY = probesCountCascadeY * cascadesCount;
    bool clear = false;
    if (ddgiData.CascadesCount != cascadesCount || Math::NotNearEqual(ddgiData.Cascades[0].ProbesSpacing, probesSpacing) || ddgiData.ProbeCounts != probesCounts || ddgiData.ProbeRaysCount != probeRaysCount)
    {
        PROFILE_CPU_NAMED("Init");
        ddgiData.Release();
        ddgiData.CascadesCount = cascadesCount;
        ddgiData.ProbeRaysCount = probeRaysCount;
        ddgiData.ProbesCountTotal = probesCountTotal;
        ddgiData.ProbeCounts = probesCounts;
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            auto& cascade = ddgiData.Cascades[cascadeIndex];
            cascade.Clear();
            cascade.ProbesSpacing = probesSpacings[cascadeIndex];
            cascade.ProbesOrigin = viewOrigins[cascadeIndex];
        }

        // Allocate probes textures
        uint64 memUsage = 0;
        auto desc = GPUTextureDescription::New2D(probesCountTotalX, probesCountTotalY, PixelFormat::Unknown);
#define INIT_TEXTURE(texture, format, width, height) desc.Format = format; desc.Width = width; desc.Height = height; ddgiData.texture = RenderTargetPool::Get(desc); if (!ddgiData.texture) return true; memUsage += ddgiData.texture->GetMemoryUsage(); RENDER_TARGET_POOL_SET_NAME(ddgiData.texture, "DDGI." #texture)
        desc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess;
        INIT_TEXTURE(ProbesTrace, PixelFormat::R16G16B16A16_Float, probeRaysCount, Math::Min(probesCountCascade, DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT));
        INIT_TEXTURE(ProbesData, PixelFormat::R8G8B8A8_SNorm, probesCountTotalX, probesCountTotalY);
        // TODO: add BC6H compression to probes data (https://github.com/knarkowicz/GPURealTimeBC6H)
        INIT_TEXTURE(ProbesIrradiance, PixelFormat::R11G11B10_Float, probesCountTotalX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), probesCountTotalY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2));
        INIT_TEXTURE(ProbesDistance, PixelFormat::R16G16_Float, probesCountTotalX * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), probesCountTotalY * (DDGI_PROBE_RESOLUTION_DISTANCE + 2));
#if DDGI_DEBUG_INSTABILITY
        INIT_TEXTURE(ProbesInstability, PixelFormat::R16_Float, probesCountTotalX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), probesCountTotalY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2));
#endif
#undef INIT_TEXTURE
#define INIT_BUFFER(buffer, name) ddgiData.buffer = GPUDevice::Instance->CreateBuffer(TEXT(name)); if (!ddgiData.buffer || ddgiData.buffer->Init(desc2)) return true; memUsage += ddgiData.buffer->GetMemoryUsage();
        GPUBufferDescription desc2 = GPUBufferDescription::Raw((probesCountCascade + 1) * sizeof(uint32), GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess);
        INIT_BUFFER(ActiveProbes, "DDGI.ActiveProbes");
        desc2 = GPUBufferDescription::Buffer(sizeof(GPUDispatchIndirectArgs) * Math::DivideAndRoundUp(probesCountCascade, DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT), GPUBufferFlags::Argument | GPUBufferFlags::UnorderedAccess, PixelFormat::R32_UInt, nullptr, sizeof(uint32));
        INIT_BUFFER(UpdateProbesInitArgs, "DDGI.UpdateProbesInitArgs");
#if DDGI_DEBUG_STATS
        desc2 = GPUBufferDescription::Raw(sizeof(StatsData), GPUBufferFlags::UnorderedAccess);
        INIT_BUFFER(StatsWrite, "DDGI.StatsWrite");
        desc2 = desc2.ToStagingReadback();
        INIT_BUFFER(StatsRead, "DDGI.StatsRead");
#endif
#undef INIT_BUFFER
        LOG(Info, "Dynamic Diffuse Global Illumination probes: {0}, memory usage: {1} MB", probesCountTotal, memUsage / (1024 * 1024));
        clear = true;
    }
#if COMPILE_WITH_DEV_ENV
    clear |= ddgiData.LastFrameUsed <= LastFrameShaderReload;
#endif
    if (clear)
    {
        // Clear probes
        PROFILE_GPU("Clear");
        context->ClearUA(ddgiData.ProbesData, Float4::Zero);
        context->ClearUA(ddgiData.ProbesIrradiance, Float4::Zero);
        context->ClearUA(ddgiData.ProbesDistance, Float4::Zero);
#if DDGI_DEBUG_INSTABILITY
        context->ClearUA(ddgiData.ProbesInstability, Float4::Zero);
#endif
    }
    ddgiData.LastFrameUsed = Engine::FrameCount;

    // Calculate which cascades should be updated this frame
    const uint64 cascadeFrequencies[] = { 2, 3, 5, 7 };
    //const uint64 cascadeFrequencies[] = { 1, 2, 3, 5 };
    //const uint64 cascadeFrequencies[] = { 1, 1, 1, 1 };
    //const uint64 cascadeFrequencies[] = { 10, 10, 10, 10 };
    bool cascadeSkipUpdate[4];
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        cascadeSkipUpdate[cascadeIndex] = !clear && (ddgiData.LastFrameUsed % cascadeFrequencies[cascadeIndex]) != 0 && GPU_SPREAD_WORKLOAD;
    }

    // Compute scrolling (probes are placed around camera but are scrolling to increase stability during movement)
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        if (cascadeSkipUpdate[cascadeIndex])
            continue;
        auto& cascade = ddgiData.Cascades[cascadeIndex];

        // Calculate the count of grid cells between the view origin and the scroll anchor
        const Float3 volumeOrigin = cascade.ProbesOrigin + Float3(cascade.ProbeScrollOffsets) * cascade.ProbesSpacing;
        const Float3 translation = viewOrigins[cascadeIndex] - volumeOrigin;
        for (int32 axis = 0; axis < 3; axis++)
        {
            const float value = translation.Raw[axis] / cascade.ProbesSpacing;
            const int32 scroll = value >= 0.0f ? (int32)Math::Floor(value) : (int32)Math::Ceil(value);
            cascade.ProbeScrollOffsets.Raw[axis] += scroll;
            cascade.ProbeScrollClears.Raw[axis] = scroll;
        }

        // Shift the volume origin based on scroll offsets for each axis once it overflows
        for (int32 axis = 0; axis < 3; axis++)
        {
            const int32 probeCount = ddgiData.ProbeCounts.Raw[axis];
            int32& scrollOffset = cascade.ProbeScrollOffsets.Raw[axis];
            while (scrollOffset >= probeCount)
            {
                cascade.ProbesOrigin.Raw[axis] += cascade.ProbesSpacing * probeCount;
                scrollOffset -= probeCount;
            }
            while (scrollOffset <= -probeCount)
            {
                cascade.ProbesOrigin.Raw[axis] -= cascade.ProbesSpacing * probeCount;
                scrollOffset += probeCount;
            }
        }
    }

    // Upload constants
    {
        ddgiData.Result.Constants.CascadesCount = cascadesCount;
        ddgiData.Result.Constants.ProbesCounts[0] = probesCounts.X;
        ddgiData.Result.Constants.ProbesCounts[1] = probesCounts.Y;
        ddgiData.Result.Constants.ProbesCounts[2] = probesCounts.Z;
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            auto& cascade = ddgiData.Cascades[cascadeIndex];
            ddgiData.Result.Constants.ProbesOriginAndSpacing[cascadeIndex] = Float4(cascade.ProbesOrigin, cascade.ProbesSpacing);
            ddgiData.Result.Constants.ProbesScrollOffsets[cascadeIndex] = Int4(cascade.ProbeScrollOffsets, 0);
        }
        ddgiData.Result.Constants.RayMaxDistance = distance;
        ddgiData.Result.Constants.ViewPos = renderContext.View.Position;
        ddgiData.Result.Constants.RaysCount = probeRaysCount;
        ddgiData.Result.Constants.ProbeHistoryWeight = probeHistoryWeight;
        ddgiData.Result.Constants.IrradianceGamma = 1.5f;
        ddgiData.Result.Constants.IndirectLightingIntensity = indirectLightingIntensity;
        ddgiData.Result.Constants.FallbackIrradiance = fallbackIrradiance.ToFloat3() * fallbackIrradiance.A;
        ddgiData.Result.ProbesData = ddgiData.ProbesData->View();
        ddgiData.Result.ProbesDistance = ddgiData.ProbesDistance->View();
        ddgiData.Result.ProbesIrradiance = ddgiData.ProbesIrradiance->View();

        Data0 data;

        // Compute random rotation matrix for probe rays orientation (randomized every frame)
        Matrix3x3 raysRotationMatrix;
        CalculateVolumeRandomRotation(raysRotationMatrix);
        Quaternion& raysRotation = *(Quaternion*)&data.RaysRotation;
        Quaternion::RotationMatrix(raysRotationMatrix, raysRotation);
        raysRotation.Conjugate();

        data.DDGI = ddgiData.Result.Constants;
        data.GlobalSDF = bindingDataSDF.Constants;
        data.GlobalSurfaceAtlas = bindingDataSurfaceAtlas.Constants;
        data.ProbesCount = data.DDGI.ProbesCounts[0] * data.DDGI.ProbesCounts[1] * data.DDGI.ProbesCounts[2];
        data.ResetBlend = clear ? 1.0f : 0.0f;
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            auto& cascade = ddgiData.Cascades[cascadeIndex];
            data.ProbeScrollClears[cascadeIndex] = Int4(cascade.ProbeScrollClears, 0);
        }
        data.TemporalTime = renderContext.List->Setup.UseTemporalAAJitter ? RenderTools::ComputeTemporalTime() : 0.0f;
        data.ViewDir = renderContext.View.Direction;
        data.SkyboxIntensity = renderContext.List->Sky ? renderContext.List->Sky->GetIndirectLightingIntensity() : 1.0f;
        GBufferPass::SetInputs(renderContext.View, data.GBuffer);
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }

    // Update probes
    {
        PROFILE_GPU_CPU_NAMED("Probes Update");
        uint32 threadGroupsX;
#if DDGI_DEBUG_STATS
        uint32 zero[4] = {};
        context->ClearUA(ddgiData.StatsWrite, zero);
#endif
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            if (cascadeSkipUpdate[cascadeIndex])
                continue;

            // Classify probes (activation/deactivation and relocation)
            {
                PROFILE_GPU_CPU_NAMED("Classify Probes");
                uint32 activeProbesCount = 0;
                context->UpdateBuffer(ddgiData.ActiveProbes, &activeProbesCount, sizeof(uint32), 0);
                threadGroupsX = Math::DivideAndRoundUp(probesCountCascade, DDGI_PROBE_CLASSIFY_GROUP_SIZE);
                context->BindSR(0, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
                context->BindSR(1, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
                context->BindUA(0, ddgiData.Result.ProbesData);
                context->BindUA(1, ddgiData.ActiveProbes->View());
                Data1 data;
                data.CascadeIndex = cascadeIndex;
                context->UpdateCB(_cb1, &data);
                context->BindCB(1, _cb1);
                context->Dispatch(_csClassify, threadGroupsX, 1, 1);
                context->ResetUA();
                context->ResetSR();
            }

            // Build indirect args for probes updating (loop over active-only probes)
            {
                PROFILE_GPU_CPU_NAMED("Init Args");
                context->BindSR(0, ddgiData.ActiveProbes->View());
                context->BindUA(0, ddgiData.UpdateProbesInitArgs->View());
                context->Dispatch(_csUpdateProbesInitArgs, 1, 1, 1);
                context->ResetUA();
            }

            // Update probes in batches so ProbesTrace texture can be smaller
            uint32 arg = 0;
            // TODO: use rays allocator to dispatch raytracing in packets (eg. 8 threads in a group instead of hardcoded limit)
            for (int32 probesOffset = 0; probesOffset < probesCountCascade; probesOffset += DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT)
            {
                Data1 data;
                data.CascadeIndex = cascadeIndex;
                data.ProbeIndexOffset = probesOffset;
                context->UpdateCB(_cb1, &data);
                context->BindCB(1, _cb1);

                // Trace rays from probes
                {
                    PROFILE_GPU_CPU_NAMED("Trace Rays");

                    // Global SDF with Global Surface Atlas software raytracing (thread X - per probe ray, thread Y - per probe)
                    context->BindSR(0, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
                    context->BindSR(1, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
                    context->BindSR(2, bindingDataSurfaceAtlas.Chunks ? bindingDataSurfaceAtlas.Chunks->View() : nullptr);
                    context->BindSR(3, bindingDataSurfaceAtlas.CulledObjects ? bindingDataSurfaceAtlas.CulledObjects->View() : nullptr);
                    context->BindSR(4, bindingDataSurfaceAtlas.Objects ? bindingDataSurfaceAtlas.Objects->View() : nullptr);
                    context->BindSR(5, bindingDataSurfaceAtlas.AtlasDepth->View());
                    context->BindSR(6, bindingDataSurfaceAtlas.AtlasLighting->View());
                    context->BindSR(7, ddgiData.Result.ProbesData);
                    context->BindSR(8, skybox);
                    context->BindSR(9, ddgiData.ActiveProbes->View());
                    context->BindUA(0, ddgiData.ProbesTrace->View());
#if DDGI_DEBUG_STATS
                    context->BindUA(1, ddgiData.StatsWrite->View());
#endif
                    context->DispatchIndirect(_csTraceRays[(int32)Graphics::GIQuality], ddgiData.UpdateProbesInitArgs, arg);
                    context->ResetUA();
                    context->ResetSR();
                }

                // Update probes irradiance and distance textures (one thread-group per probe)
                {
                    PROFILE_GPU_CPU_NAMED("Update Probes");

                    // Distance
                    context->BindSR(0, ddgiData.Result.ProbesData);
                    context->BindSR(1, ddgiData.ProbesTrace->View());
                    context->BindSR(2, ddgiData.ActiveProbes->View());
                    context->BindUA(0, ddgiData.Result.ProbesDistance);
                    context->DispatchIndirect(_csUpdateProbesDistance, ddgiData.UpdateProbesInitArgs, arg);
                    context->ResetUA();
                    context->ResetSR();

                    // Irradiance
                    context->BindSR(1, ddgiData.ProbesTrace->View());
                    context->BindSR(2, ddgiData.ActiveProbes->View());
                    context->BindUA(0, ddgiData.Result.ProbesIrradiance);
                    context->BindUA(1, ddgiData.Result.ProbesData);
#if DDGI_DEBUG_INSTABILITY
                    context->BindUA(2, ddgiData.ProbesInstability->View());
#endif
                    context->DispatchIndirect(_csUpdateProbesIrradiance, ddgiData.UpdateProbesInitArgs, arg);
                    context->ResetUA();
                    context->ResetSR();
                }

                arg += sizeof(GPUDispatchIndirectArgs);
            }
        }

#if DDGI_DEBUG_STATS
        // Update stats
        {
            StatsData stats;
            if (void* mapped = ddgiData.StatsRead->Map(GPUResourceMapMode::Read))
            {
                Platform::MemoryCopy(&stats, mapped, sizeof(stats));
                ddgiData.StatsRead->Unmap();
                ddgiData.StatsProbes.Add(stats.ProbesCount);
                ddgiData.StatsRays.Add(stats.RaysCount);
            }
            context->CopyBuffer(ddgiData.StatsRead, ddgiData.StatsWrite, sizeof(stats));
            if (++ddgiData.StatsFrames >= DDGI_DEBUG_STATS_FRAMES)
            {
                ddgiData.StatsFrames = 0;
                stats.ProbesCount = ddgiData.StatsProbes.Average();
                stats.RaysCount = ddgiData.StatsRays.Average();
                LOG(Info, "DDGI active probes: {}, traced rays: {} per frame, rays per probe: {}", stats.ProbesCount, stats.RaysCount, stats.ProbesCount > 0 ? stats.RaysCount / stats.ProbesCount : 0);
            }
        }
#endif
    }

    return false;
}

bool DynamicDiffuseGlobalIlluminationPass::Render(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer)
{
    if (checkIfSkipPass())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    RenderBuffers* renderBuffers = renderContext.Buffers;
    bool render = true;
    if (renderContext.View.IsOfflinePass)
    {
        // During offline pass (eg. probes rendering) we can try reuse main game viewport or editor viewport DDGI probes
        // TODO: apply it for transparency too (in DynamicDiffuseGlobalIlluminationPass::Get)
        for (auto* task : RenderTask::Tasks)
        {
            if (auto* sceneTask = ScriptingObject::Cast<SceneRenderTask>(task))
            {
                auto* sceneTaskDDGI = sceneTask->Buffers ? sceneTask->Buffers->FindCustomBuffer<DDGICustomBuffer>(TEXT("DDGI")) : nullptr;
                if (sceneTaskDDGI && sceneTaskDDGI->LastFrameUsed + 1 >= Engine::FrameCount)
                {
                    // Reuse DDGI from this task
                    renderBuffers = sceneTask->Buffers;
                    render = false;
                    break;
                }
            }
        }
    }
    auto& ddgiData = *renderBuffers->GetCustomBuffer<DDGICustomBuffer>(TEXT("DDGI"));
    if (render && ddgiData.LastFrameUsed == Engine::FrameCount)
        render = false;
    PROFILE_GPU_CPU("Dynamic Diffuse Global Illumination");

    if (render)
    {
        if (RenderInner(renderContext, context, ddgiData))
        {
            context->ResetRenderTarget();
            context->ResetSR();
            context->ResetUA();
            context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
            return true;
        }
    }

    // Render indirect lighting
    if (lightBuffer)
    {
        PROFILE_GPU_CPU_NAMED("Indirect Lighting");
#if 0
        // DDGI indirect lighting debug preview
        context->Clear(lightBuffer, Color::Transparent);
#endif
        if (!render)
        {
            Data0 data;
            data.DDGI = ddgiData.Result.Constants;
            data.TemporalTime = 0.0f;
            GBufferPass::SetInputs(renderContext.View, data.GBuffer);
            context->UpdateCB(_cb0, &data);
            context->BindCB(0, _cb0);
        }
        context->BindSR(0, renderContext.Buffers->GBuffer0->View());
        context->BindSR(1, renderContext.Buffers->GBuffer1->View());
        context->BindSR(2, renderContext.Buffers->GBuffer2->View());
        context->BindSR(3, renderContext.Buffers->DepthBuffer->View());
        context->BindSR(4, ddgiData.Result.ProbesData);
        context->BindSR(5, ddgiData.Result.ProbesDistance);
        context->BindSR(6, ddgiData.Result.ProbesIrradiance);
        context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
        context->SetRenderTarget(lightBuffer);
        context->SetState(_psIndirectLighting[Graphics::GICascadesBlending ? 1 : 0]);
        context->DrawFullscreenTriangle();
    }

#if USE_EDITOR
    // Probes debug drawing
    if (renderContext.View.Mode == ViewMode::GlobalIllumination && lightBuffer)
    {
        PROFILE_GPU_CPU_NAMED("Debug Probes");
        if (!_debugModel)
            _debugModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Primitives/Sphere"));
        if (!_debugMaterial)
            _debugMaterial = Content::LoadAsyncInternal<MaterialBase>(TEXT("Editor/DebugMaterials/DDGIDebugProbes"));
        if (_debugModel && _debugModel->IsLoaded() && _debugModel->CanBeRendered() && _debugMaterial && _debugMaterial->IsLoaded())
        {
            RenderContext debugRenderContext(renderContext);
            Matrix world;
            Matrix::Scaling(Float3(0.2f), world);
            const Mesh& debugMesh = _debugModel->LODs[0].Meshes[0];
            constexpr int32 maxProbesPerDrawing = 32 * 1024;
            int32 probesDrawingStart = 0;
            while (probesDrawingStart < ddgiData.ProbesCountTotal)
            {
                debugRenderContext.List = RenderList::GetFromPool();
                debugRenderContext.View.Pass = DrawPass::GBuffer;
                debugRenderContext.View.Prepare(debugRenderContext);
                // TODO: refactor this into BatchedDrawCalls for faster draw calls processing
                const int32 probesDrawingEnd = Math::Min(probesDrawingStart + maxProbesPerDrawing, ddgiData.ProbesCountTotal);
                for (int32 probeIndex = probesDrawingStart; probeIndex < probesDrawingEnd; probeIndex++)
                    debugMesh.Draw(debugRenderContext, _debugMaterial, world, StaticFlags::None, true, DrawPass::GBuffer, (float)probeIndex);
                debugRenderContext.List->SortDrawCalls(debugRenderContext, false, DrawCallsListType::GBuffer);
                context->SetViewportAndScissors(debugRenderContext.View.ScreenSize.X, debugRenderContext.View.ScreenSize.Y);
                GPUTextureView* targetBuffers[5] =
                {
                    lightBuffer,
                    renderContext.Buffers->GBuffer0->View(),
                    renderContext.Buffers->GBuffer1->View(),
                    renderContext.Buffers->GBuffer2->View(),
                    renderContext.Buffers->GBuffer3->View(),
                };
                context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
                {
                    // Pass DDGI data to the material
                    _debugMaterial->SetParameterValue(TEXT("ProbesData"), Variant(ddgiData.ProbesData));
    #if DDGI_DEBUG_INSTABILITY
                    _debugMaterial->SetParameterValue(TEXT("ProbesIrradiance"), Variant(ddgiData.ProbesInstability));
    #else
                    _debugMaterial->SetParameterValue(TEXT("ProbesIrradiance"), Variant(ddgiData.ProbesIrradiance));
    #endif
                    _debugMaterial->SetParameterValue(TEXT("ProbesDistance"), Variant(ddgiData.ProbesDistance));
                    auto cb = _debugMaterial->GetShader()->GetCB(3);
                    if (cb)
                    {
                        context->UpdateCB(cb, &ddgiData.Result.Constants);
                        context->BindCB(3, cb);
                    }
                }
                debugRenderContext.List->ExecuteDrawCalls(debugRenderContext, DrawCallsListType::GBuffer);
                RenderList::ReturnToPool(debugRenderContext.List);
                probesDrawingStart = probesDrawingEnd;
            }
        }
    }
#endif

    context->ResetRenderTarget();
    context->ResetSR();
    context->ResetUA();
    context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
    return false;
}
