// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "DynamicDiffuseGlobalIllumination.h"
#include "GlobalSurfaceAtlasPass.h"
#include "../GlobalSignDistanceFieldPass.h"
#include "../RenderList.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
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
#define DDGI_TRACE_RAYS_GROUP_SIZE_X 32
#define DDGI_TRACE_RAYS_LIMIT 512 // Limit of rays per-probe (runtime value can be smaller)
#define DDGI_PROBE_RESOLUTION_IRRADIANCE 6 // Resolution (in texels) for probe irradiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_DISTANCE 14 // Resolution (in texels) for probe distance data (excluding 1px padding on each side)
#define DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE 8
#define DDGI_PROBE_CLASSIFY_GROUP_SIZE 32

PACK_STRUCT(struct Data0
    {
    DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
    GlobalSurfaceAtlasPass::ConstantsData GlobalSurfaceAtlas;
    GBufferData GBuffer;
    float ResetBlend;
    float TemporalTime;
    float IndirectLightingIntensity;
    float Padding0;
    });

PACK_STRUCT(struct Data1
    {
    Vector3 Padding1;
    uint32 CascadeIndex; // TODO: use push constants on Vulkan or root signature data on DX12 to reduce overhead of changing single DWORD
    });

class DDGICustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    struct
    {
        Vector3 ProbesOrigin;
        float ProbesSpacing = 0.0f;
        Int3 ProbeScrollOffsets;
        Int3 ProbeScrollDirections;
        bool ProbeScrollClear[3];

        void Clear()
        {
            ProbesOrigin = Vector3::Zero;
            ProbeScrollOffsets = Int3::Zero;
            ProbeScrollDirections = Int3::Zero;
            ProbeScrollClear[0] = false;
            ProbeScrollClear[1] = false;
            ProbeScrollClear[2] = false;
        }
    } Cascades[4];

    int32 CascadesCount = 0;
    int32 ProbeRaysCount = 0;
    Int3 ProbeCounts = Int3::Zero;
    GPUTexture* ProbesTrace = nullptr; // Probes ray tracing: (RGB: hit radiance, A: hit distance)
    GPUTexture* ProbesState = nullptr; // Probes state: (RGB: world-space offset, A: state)
    GPUTexture* ProbesIrradiance = nullptr; // Probes irradiance (RGB: sRGB color)
    GPUTexture* ProbesDistance = nullptr; // Probes distance (R: mean distance, G: mean distance^2)
    DynamicDiffuseGlobalIlluminationPass::BindingData Result;

    FORCE_INLINE void Release()
    {
        RenderTargetPool::Release(ProbesTrace);
        RenderTargetPool::Release(ProbesState);
        RenderTargetPool::Release(ProbesIrradiance);
        RenderTargetPool::Release(ProbesDistance);
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
    _csTraceRays = shader->GetCS("CS_TraceRays");
    _csUpdateProbesIrradiance = shader->GetCS("CS_UpdateProbes", 0);
    _csUpdateProbesDistance = shader->GetCS("CS_UpdateProbes", 1);
    _csUpdateBordersIrradianceRow = shader->GetCS("CS_UpdateBorders", 0);
    _csUpdateBordersIrradianceCollumn = shader->GetCS("CS_UpdateBorders", 1);
    _csUpdateBordersDistanceRow = shader->GetCS("CS_UpdateBorders", 2);
    _csUpdateBordersDistanceCollumn = shader->GetCS("CS_UpdateBorders", 3);
    auto device = GPUDevice::Instance;
    auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psIndirectLighting)
    {
        _psIndirectLighting = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_IndirectLighting");
        psDesc.BlendMode = BlendingMode::Additive;
        if (_psIndirectLighting->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void DynamicDiffuseGlobalIlluminationPass::OnShaderReloading(Asset* obj)
{
    LastFrameShaderReload = Engine::FrameCount;
    _csClassify = nullptr;
    _csTraceRays = nullptr;
    _csUpdateProbesIrradiance = nullptr;
    _csUpdateProbesDistance = nullptr;
    _csUpdateBordersIrradianceRow = nullptr;
    _csUpdateBordersIrradianceCollumn = nullptr;
    _csUpdateBordersDistanceRow = nullptr;
    _csUpdateBordersDistanceCollumn = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting);
    invalidateResources();
}

#endif

void DynamicDiffuseGlobalIlluminationPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    _cb0 = nullptr;
    _cb1 = nullptr;
    _csTraceRays = nullptr;
    _shader = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting);
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

bool DynamicDiffuseGlobalIlluminationPass::Render(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer)
{
    // Skip if not supported
    if (checkIfSkipPass())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& ddgiData = *renderContext.Buffers->GetCustomBuffer<DDGICustomBuffer>(TEXT("DDGI"));

    // Render Global SDF and Global Surface Atlas for software raytracing
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF))
        return true;
    GlobalSurfaceAtlasPass::BindingData bindingDataSurfaceAtlas;
    if (GlobalSurfaceAtlasPass::Instance()->Render(renderContext, context, bindingDataSurfaceAtlas))
        return true;
    GPUTextureView* skybox = GBufferPass::Instance()->RenderSkybox(renderContext, context);

    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (ddgiData.LastFrameUsed == currentFrame)
        return false;
    ddgiData.LastFrameUsed = currentFrame;
    PROFILE_GPU_CPU("Dynamic Diffuse Global Illumination");

    // TODO: configurable via graphics settings
    const Quality quality = Quality::Ultra;
    bool debugProbes = false; // TODO: add debug option to draw probes locations -> in Graphics window - Editor-only
    // TODO: configurable via postFx settings (maybe use Global SDF distance?)
    const float indirectLightingIntensity = 1.0f;
    const float probeHistoryWeight = 0.8f;
    const int32 cascadesCount = 4; // in range 1-4
    // TODO: use GI.Distance as a easier to adjust total distance and automatically calculate distanceExtent from it
    const float distance = 20000.0f; // GI distance around the view (in each direction)
    const float cascadesDistanceScales[] = { 1.0f, 3.0f, 6.0f, 10.0f }; // Scales each cascade further away from the camera origin
    const float distanceExtent = distance / cascadesDistanceScales[cascadesCount - 1];
    const float verticalRangeScale = 0.8f; // Scales the probes volume size at Y axis (horizontal aspect ratio makes the DDGI use less probes vertically to cover whole screen)
    const float probesSpacing = 200.0f; // GI probes placement spacing nearby camera (for closest cascade; gets automatically reduced for further cascades)
    const Color fallbackIrradiance = Color::Black; // Irradiance lighting outside the DDGI range used as a fallback to prevent pure-black scene outside the GI range
    const Int3 probesCounts(Vector3::Ceil(Vector3(distanceExtent, distanceExtent * verticalRangeScale, distanceExtent) / probesSpacing));
    const int32 probeRaysCount = Math::Min(Math::AlignUp(256, DDGI_TRACE_RAYS_GROUP_SIZE_X), DDGI_TRACE_RAYS_LIMIT); // TODO: make it based on the GI Quality

    // Initialize cascades
    float probesSpacings[4];
    Vector3 viewOrigins[4];
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        // Each cascade has higher spacing between probes
        float cascadeDistanceScale = cascadesDistanceScales[cascadeIndex];
        float cascadeProbesSpacing = probesSpacing * cascadeDistanceScale;
        probesSpacings[cascadeIndex] = cascadeProbesSpacing;

        // Calculate view origin for cascade by shifting it towards the view direction to account for better view frustum coverage
        Vector3 viewOrigin = renderContext.View.Position;
        Vector3 viewDirection = renderContext.View.Direction;
        const Vector3 probesDistance = Vector3(probesCounts) * cascadeProbesSpacing;
        const float probesDistanceMax = probesDistance.MaxValue();
        const Vector2 viewRayHit = CollisionsHelper::LineHitsBox(viewOrigin, viewOrigin + viewDirection * (probesDistanceMax * 2.0f), viewOrigin - probesDistance, viewOrigin + probesDistance);
        const float viewOriginOffset = viewRayHit.Y * probesDistanceMax * 0.6f;
        viewOrigin += viewDirection * viewOriginOffset;
        const float viewOriginSnapping = cascadeProbesSpacing;
        viewOrigin = Vector3::Floor(viewOrigin / viewOriginSnapping) * viewOriginSnapping;
        //viewOrigin = Vector3::Zero;
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
        // TODO rethink probes data placement in memory -> what if we get [50x50x30] resolution? That's 75000 probes! Use sparse storage with active-only probes
#define INIT_TEXTURE(texture, format, width, height) desc.Format = format; desc.Width = width; desc.Height = height; ddgiData.texture = RenderTargetPool::Get(desc); if (!ddgiData.texture) return true; memUsage += ddgiData.texture->GetMemoryUsage()
        desc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess;
        INIT_TEXTURE(ProbesTrace, PixelFormat::R16G16B16A16_Float, probeRaysCount, probesCountTotal); // TODO: limit to 4k probes for a single batch to trace
        INIT_TEXTURE(ProbesState, PixelFormat::R16G16B16A16_Float, probesCountTotalX, probesCountTotalY); // TODO: optimize to a RGBA32 (pos offset can be normalized to [0-0.5] range of ProbesSpacing and packed with state flag)
        INIT_TEXTURE(ProbesIrradiance, PixelFormat::R11G11B10_Float, probesCountTotalX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), probesCountTotalY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2));
        INIT_TEXTURE(ProbesDistance, PixelFormat::R16G16_Float, probesCountTotalX * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), probesCountTotalY * (DDGI_PROBE_RESOLUTION_DISTANCE + 2));
#undef INIT_TEXTURE
        LOG(Info, "Dynamic Diffuse Global Illumination memory usage: {0} MB, probes: {1}", memUsage / 1024 / 1024, probesCountTotal);
        clear = true;
    }
#if USE_EDITOR
    clear |= ddgiData.LastFrameUsed <= LastFrameShaderReload;
#endif
    if (clear)
    {
        // Clear probes
        PROFILE_GPU("Clear");
        context->ClearUA(ddgiData.ProbesState, Vector4::Zero);
        context->ClearUA(ddgiData.ProbesIrradiance, Vector4::Zero);
        context->ClearUA(ddgiData.ProbesDistance, Vector4::Zero);
    }

    // Calculate which cascades should be updated this frame
    //const uint64 cascadeFrequencies[] = { 1, 2, 3, 5 };
    // TODO: prevent updating 2 cascades at once on Low quality
    const uint64 cascadeFrequencies[] = { 1, 1, 1, 1 };
    bool cascadeSkipUpdate[4];
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        cascadeSkipUpdate[cascadeIndex] = !clear && (currentFrame % cascadeFrequencies[cascadeIndex]) != 0;
    }

    // Compute scrolling (probes are placed around camera but are scrolling to increase stability during movement)
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        if (cascadeSkipUpdate[cascadeIndex])
            continue;
        auto& cascade = ddgiData.Cascades[cascadeIndex];

        // Reset the volume origin and scroll offsets for each axis
        for (int32 axis = 0; axis < 3; axis++)
        {
            if (cascade.ProbeScrollOffsets.Raw[axis] != 0 && (cascade.ProbeScrollOffsets.Raw[axis] % ddgiData.ProbeCounts.Raw[axis] == 0))
            {
                cascade.ProbesOrigin.Raw[axis] += (float)ddgiData.ProbeCounts.Raw[axis] * cascade.ProbesSpacing * (float)cascade.ProbeScrollDirections.Raw[axis];
                cascade.ProbeScrollOffsets.Raw[axis] = 0;
            }
        }

        // Calculate the count of grid cells between the view origin and the scroll anchor
        const Vector3 volumeOrigin = cascade.ProbesOrigin + Vector3(cascade.ProbeScrollOffsets) * cascade.ProbesSpacing;
        const Vector3 translation = viewOrigins[cascadeIndex] - volumeOrigin;
        for (int32 axis = 0; axis < 3; axis++)
        {
            const float value = translation.Raw[axis] / cascade.ProbesSpacing;
            const int32 scroll = value >= 0.0f ? (int32)Math::Floor(value) : (int32)Math::Ceil(value);
            cascade.ProbeScrollOffsets.Raw[axis] += scroll;
            cascade.ProbeScrollClear[axis] = scroll != 0;
            cascade.ProbeScrollDirections.Raw[axis] = translation.Raw[axis] >= 0.0f ? 1 : -1;
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
            int32 probeScrollClear = cascade.ProbeScrollClear[0] + cascade.ProbeScrollClear[1] * 2 + cascade.ProbeScrollClear[2] * 4; // Pack clear flags into bits
            ddgiData.Result.Constants.ProbesOriginAndSpacing[cascadeIndex] = Vector4(cascade.ProbesOrigin, cascade.ProbesSpacing);
            ddgiData.Result.Constants.ProbesScrollOffsets[cascadeIndex] = Int4(cascade.ProbeScrollOffsets, probeScrollClear);
            ddgiData.Result.Constants.ProbeScrollDirections[cascadeIndex] = Int4(cascade.ProbeScrollDirections, 0);
        }
        ddgiData.Result.Constants.RayMaxDistance = 10000.0f; // TODO: adjust to match perf/quality ratio (make it based on Global SDF and Global Surface Atlas distance)
        ddgiData.Result.Constants.ViewDir = renderContext.View.Direction;
        ddgiData.Result.Constants.RaysCount = probeRaysCount;
        ddgiData.Result.Constants.ProbeHistoryWeight = probeHistoryWeight;
        ddgiData.Result.Constants.IrradianceGamma = 5.0f;
        ddgiData.Result.Constants.FallbackIrradiance = fallbackIrradiance.ToVector3() * fallbackIrradiance.A;
        ddgiData.Result.ProbesState = ddgiData.ProbesState->View();
        ddgiData.Result.ProbesDistance = ddgiData.ProbesDistance->View();
        ddgiData.Result.ProbesIrradiance = ddgiData.ProbesIrradiance->View();

        // Compute random rotation matrix for probe rays orientation (randomized every frame)
        Matrix3x3 raysRotationMatrix;
        CalculateVolumeRandomRotation(raysRotationMatrix);
        Quaternion& raysRotation = *(Quaternion*)&ddgiData.Result.Constants.RaysRotation;
        Quaternion::RotationMatrix(raysRotationMatrix, raysRotation);
        raysRotation.Conjugate();

        Data0 data;
        data.DDGI = ddgiData.Result.Constants;
        data.GlobalSDF = bindingDataSDF.Constants;
        data.GlobalSurfaceAtlas = bindingDataSurfaceAtlas.Constants;
        data.ResetBlend = clear ? 1.0f : 0.0f;
        if (renderContext.List->Settings.AntiAliasing.Mode == AntialiasingMode::TemporalAntialiasing)
        {
            // Use temporal offset in the dithering factor (gets cleaned out by TAA)
            const float time = Time::Draw.UnscaledTime.GetTotalSeconds();
            const float scale = 10;
            const float integral = roundf(time / scale) * scale;
            data.TemporalTime = time - integral;
        }
        else
        {
            data.TemporalTime = 0.0f;
        }
        data.IndirectLightingIntensity = indirectLightingIntensity;
        GBufferPass::SetInputs(renderContext.View, data.GBuffer);
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }

    // Classify probes (activation/deactivation and relocation)
    {
        PROFILE_GPU_CPU("Probes Classification");
        uint32 threadGroups = Math::DivideAndRoundUp(probesCountCascade, DDGI_PROBE_CLASSIFY_GROUP_SIZE);
        for (int32 i = 0; i < 4; i++)
        {
            context->BindSR(i, bindingDataSDF.Cascades[i]->ViewVolume());
        }
        context->BindUA(0, ddgiData.Result.ProbesState);
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            if (cascadeSkipUpdate[cascadeIndex])
                continue;
            Data1 data;
            data.CascadeIndex = cascadeIndex;
            context->UpdateCB(_cb1, &data);
            context->BindCB(1, _cb1);
            context->Dispatch(_csClassify, threadGroups, 1, 1);
        }
        context->ResetUA();
    }

    // Update probes
    {
        PROFILE_GPU_CPU("Probes Update");
        bool anyDirty = false;
        uint32 threadGroupsX, threadGroupsY;
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            if (cascadeSkipUpdate[cascadeIndex])
                continue;
            anyDirty = true;
            Data1 data;
            data.CascadeIndex = cascadeIndex;
            context->UpdateCB(_cb1, &data);
            context->BindCB(1, _cb1);

            // TODO: run probes tracing+update in 4k batches

            // Trace rays from probes
            {
                PROFILE_GPU_CPU("Trace Rays");

                // Global SDF with Global Surface Atlas software raytracing (thread X - per probe ray, thread Y - per probe)
                ASSERT_LOW_LAYER((probeRaysCount % DDGI_TRACE_RAYS_GROUP_SIZE_X) == 0);
                for (int32 i = 0; i < 4; i++)
                {
                    context->BindSR(i, bindingDataSDF.Cascades[i]->ViewVolume());
                    context->BindSR(i + 4, bindingDataSDF.CascadeMips[i]->ViewVolume());
                }
                context->BindSR(8, bindingDataSurfaceAtlas.Chunks ? bindingDataSurfaceAtlas.Chunks->View() : nullptr);
                context->BindSR(9, bindingDataSurfaceAtlas.CulledObjects ? bindingDataSurfaceAtlas.CulledObjects->View() : nullptr);
                context->BindSR(10, bindingDataSurfaceAtlas.AtlasDepth->View());
                context->BindSR(11, bindingDataSurfaceAtlas.AtlasLighting->View());
                context->BindSR(12, ddgiData.Result.ProbesState);
                context->BindSR(13, skybox);
                context->BindUA(0, ddgiData.ProbesTrace->View());
                context->Dispatch(_csTraceRays, probeRaysCount / DDGI_TRACE_RAYS_GROUP_SIZE_X, probesCountCascade, 1);
                context->ResetUA();
                context->ResetSR();

#if 0
                // Probes trace debug preview
                context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
                context->SetRenderTarget(lightBuffer);
                context->Draw(ddgiData.ProbesTrace);
                return false;
#endif
            }

            context->BindSR(0, ddgiData.Result.ProbesState);
            context->BindSR(1, ddgiData.ProbesTrace->View());

            // Update probes irradiance texture
            {
                PROFILE_GPU_CPU("Update Irradiance");
                context->BindUA(0, ddgiData.Result.ProbesIrradiance);
                context->Dispatch(_csUpdateProbesIrradiance, probesCountCascadeX, probesCountCascadeY, 1);
            }

            // Update probes distance texture
            {
                PROFILE_GPU_CPU("Update Distance");
                context->BindUA(0, ddgiData.Result.ProbesDistance);
                context->Dispatch(_csUpdateProbesDistance, probesCountCascadeX, probesCountCascadeY, 1);
            }
        }

        // Update probes border pixels
        if (anyDirty)
        {
            PROFILE_GPU_CPU("Update Borders");

            // Irradiance
            context->BindUA(0, ddgiData.Result.ProbesIrradiance);
            threadGroupsX = Math::DivideAndRoundUp(probesCountTotalX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            threadGroupsY = Math::DivideAndRoundUp(probesCountTotalY, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            context->Dispatch(_csUpdateBordersIrradianceRow, threadGroupsX, threadGroupsY, 1);
            threadGroupsX = Math::DivideAndRoundUp(probesCountTotalX, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            threadGroupsY = Math::DivideAndRoundUp(probesCountTotalY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            context->Dispatch(_csUpdateBordersIrradianceCollumn, threadGroupsX, threadGroupsY, 1);

            // Distance
            context->BindUA(0, ddgiData.Result.ProbesDistance);
            threadGroupsX = Math::DivideAndRoundUp(probesCountTotalX * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            threadGroupsY = Math::DivideAndRoundUp(probesCountTotalY, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            context->Dispatch(_csUpdateBordersDistanceRow, threadGroupsX, threadGroupsY, 1);
            threadGroupsX = Math::DivideAndRoundUp(probesCountTotalX, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            threadGroupsY = Math::DivideAndRoundUp(probesCountTotalY * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
            context->Dispatch(_csUpdateBordersDistanceCollumn, threadGroupsX, threadGroupsY, 1);

            context->ResetUA();
            context->ResetSR();
        }
    }

    // Render indirect lighting
    if (lightBuffer)
    {
        PROFILE_GPU_CPU("Indirect Lighting");
#if 0
        // DDGI indirect lighting debug preview
        context->Clear(lightBuffer, Color::Transparent);
#endif
        context->BindSR(0, renderContext.Buffers->GBuffer0->View());
        context->BindSR(1, renderContext.Buffers->GBuffer1->View());
        context->BindSR(2, renderContext.Buffers->GBuffer2->View());
        context->BindSR(3, renderContext.Buffers->DepthBuffer->View());
        context->BindSR(4, ddgiData.Result.ProbesState);
        context->BindSR(5, ddgiData.Result.ProbesDistance);
        context->BindSR(6, ddgiData.Result.ProbesIrradiance);
        context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
        context->SetRenderTarget(lightBuffer);
        context->SetState(_psIndirectLighting);
        context->DrawFullscreenTriangle();
    }

#if USE_EDITOR
    // Probes debug drawing
    if (debugProbes && lightBuffer)
    {
        PROFILE_GPU_CPU("Debug Probes");
        if (!_debugModel)
            _debugModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Primitives/Sphere"));
        if (!_debugMaterial)
            _debugMaterial = Content::LoadAsyncInternal<MaterialBase>(TEXT("Editor/DebugMaterials/DDGIDebugProbes"));
        if (_debugModel && _debugModel->IsLoaded() && _debugModel->CanBeRendered() && _debugMaterial && _debugMaterial->IsLoaded())
        {
            RenderContext debugRenderContext(renderContext);
            debugRenderContext.List = RenderList::GetFromPool();
            debugRenderContext.View.Pass = DrawPass::GBuffer;
            debugRenderContext.View.Prepare(debugRenderContext);
            Matrix world;
            Matrix::Scaling(Vector3(0.2f), world);
            const Mesh& debugMesh = _debugModel->LODs[0].Meshes[0];
            for (int32 probeIndex = 0; probeIndex < probesCountTotal; probeIndex++)
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
                _debugMaterial->SetParameterValue(TEXT("ProbesState"), Variant(ddgiData.ProbesState));
                _debugMaterial->SetParameterValue(TEXT("ProbesIrradiance"), Variant(ddgiData.ProbesIrradiance));
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
            context->UnBindCB(3);
            context->ResetRenderTarget();
        }
    }
#endif

    context->ResetRenderTarget();
    context->ResetSR();

    return false;
}
