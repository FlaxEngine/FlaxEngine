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
    Vector3 Padding0;
    float IndirectLightingIntensity;
    });

class DDGICustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    int32 ProbeRaysCount = 0;
    float ProbesSpacing = 0.0f;
    Int3 ProbeCounts = Int3::Zero;
    Vector3 ProbesOrigin;
    Int3 ProbeScrollOffsets;
    Int3 ProbeScrollDirections;
    bool ProbeScrollClear[3];
    GPUTexture* ProbesTrace = nullptr; // Probes ray tracing: (RGB: hit radiance, A: hit distance)
    GPUTexture* ProbesState = nullptr; // Probes state: (RGB: world-space offset, A: state)
    GPUTexture* ProbesIrradiance = nullptr; // Probes irradiance (RGB: sRGB color)
    GPUTexture* ProbesDistance = nullptr; // Probes distance (R: mean distance, G: mean distance^2)
    DynamicDiffuseGlobalIlluminationPass::BindingData Result;

    FORCE_INLINE void Clear()
    {
        ProbesOrigin = Vector3::Zero;
        ProbeScrollOffsets = Int3::Zero;
        ProbeScrollDirections = Int3::Zero;
        ProbeScrollClear[0] = false;
        ProbeScrollClear[1] = false;
        ProbeScrollClear[2] = false;
        RenderTargetPool::Release(ProbesTrace);
        RenderTargetPool::Release(ProbesState);
        RenderTargetPool::Release(ProbesIrradiance);
        RenderTargetPool::Release(ProbesDistance);
    }

    ~DDGICustomBuffer()
    {
        Clear();
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

int32 AbsFloor(const float value)
{
    return value >= 0.0f ? (int32)Math::Floor(value) : (int32)Math::Ceil(value);
}

int32 GetSignNotZero(const float value)
{
    return value >= 0.0f ? 1 : -1;
}

Vector3 GetVolumeOrigin(DDGICustomBuffer& ddgiData)
{
    return ddgiData.ProbesOrigin + Vector3(ddgiData.ProbeScrollOffsets) * ddgiData.ProbesSpacing;
}

void CalculateVolumeScrolling(DDGICustomBuffer& ddgiData, const Vector3& viewOrigin)
{
    // Reset the volume origin and scroll offsets for each axis
    for (int32 axis = 0; axis < 3; axis++)
    {
        if (ddgiData.ProbeScrollOffsets.Raw[axis] != 0 && (ddgiData.ProbeScrollOffsets.Raw[axis] % ddgiData.ProbeCounts.Raw[axis] == 0))
        {
            ddgiData.ProbesOrigin.Raw[axis] += (float)ddgiData.ProbeCounts.Raw[axis] * ddgiData.ProbesSpacing * (float)ddgiData.ProbeScrollDirections.Raw[axis];
            ddgiData.ProbeScrollOffsets.Raw[axis] = 0;
        }
    }

    // Calculate the count of grid cells between the view origin and the scroll anchor
    const Vector3 translation = viewOrigin - GetVolumeOrigin(ddgiData);
    for (int32 axis = 0; axis < 3; axis++)
    {
        const int32 scroll = AbsFloor(translation.Raw[axis] / ddgiData.ProbesSpacing);
        ddgiData.ProbeScrollOffsets.Raw[axis] += scroll;
        ddgiData.ProbeScrollClear[axis] = scroll != 0;
        ddgiData.ProbeScrollDirections.Raw[axis] = GetSignNotZero(translation.Raw[axis]);
    }
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
    if (!_cb0)
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
    bool debugProbes = true; // TODO: add debug option to draw probes locations -> in Graphics window - Editor-only
    // TODO: configurable via postFx settings (maybe use Global SDF distance?)
    const float indirectLightingIntensity = 1.0f;
    const Vector3 giDistance(2000, 2000, 2000); // GI distance around the view (in each direction)
    const float giResolution = 100.0f; // GI probes placement spacing
    const Int3 probesCounts(Vector3::Ceil(giDistance / giResolution));
    const Vector3 probesDistance = Vector3(probesCounts) * giResolution;
    const int32 probeRaysCount = Math::Min(Math::AlignUp(256, DDGI_TRACE_RAYS_GROUP_SIZE_X), DDGI_TRACE_RAYS_LIMIT); // TODO: make it based on the GI Quality
    const float probeHistoryWeight = 0.97f;

    // Init buffers
    const int32 probesCount = probesCounts.X * probesCounts.Y * probesCounts.Z;
    if (probesCount == 0 || indirectLightingIntensity <= ZeroTolerance)
        return true;
    int32 probesCountX = probesCounts.X * probesCounts.Y;
    int32 probesCountY = probesCounts.Z;
    bool clear = false;
    if (Math::NotNearEqual(ddgiData.ProbesSpacing, giResolution) || ddgiData.ProbeCounts != probesCounts || ddgiData.ProbeRaysCount != probeRaysCount)
    {
        PROFILE_CPU_NAMED("Init");
        ddgiData.Clear();
        ddgiData.ProbeRaysCount = probeRaysCount;
        ddgiData.ProbesSpacing = giResolution;
        ddgiData.ProbeCounts = probesCounts;

        // Allocate probes textures
        uint64 memUsage = 0;
        auto desc = GPUTextureDescription::New2D(probesCountX, probesCountY, PixelFormat::Unknown);
        // TODO rethink probes data placement in memory -> what if we get [50x50x30] resolution? That's 75000 probes! Use sparse storage with active-only probes
#define INIT_TEXTURE(texture, format, width, height) desc.Format = format; desc.Width = width; desc.Height = height; ddgiData.texture = RenderTargetPool::Get(desc); if (!ddgiData.texture) return true; memUsage += ddgiData.texture->GetMemoryUsage()
        desc.Flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess;
        INIT_TEXTURE(ProbesTrace, PixelFormat::R16G16B16A16_Float, probeRaysCount, probesCount);
        INIT_TEXTURE(ProbesState, PixelFormat::R16G16B16A16_Float, probesCountX, probesCountY); // TODO: optimize to a RGBA32 (pos offset can be normalized to [0-0.5] range of ProbesSpacing and packed with state flag)
        INIT_TEXTURE(ProbesIrradiance, PixelFormat::R11G11B10_Float, probesCountX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), probesCountY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2));
        INIT_TEXTURE(ProbesDistance, PixelFormat::R16G16_Float, probesCountX * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), probesCountY * (DDGI_PROBE_RESOLUTION_DISTANCE + 2));
#undef INIT_TEXTURE
        LOG(Info, "Dynamic Diffuse Global Illumination memory usage: {0} MB, probes: {1}", memUsage / 1024 / 1024, probesCount);
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

    // Compute random rotation matrix for probe rays orientation (randomized every frame)
    Matrix3x3 raysRotationMatrix;
    CalculateVolumeRandomRotation(raysRotationMatrix);

    // Compute scrolling (probes are placed around camera but are scrolling to increase stability during movement)
    Vector3 viewOrigin = renderContext.View.Position;
    Vector3 viewDirection = renderContext.View.Direction;
    const float probesDistanceMax = probesDistance.MaxValue();
    const Vector2 viewRayHit = CollisionsHelper::LineHitsBox(viewOrigin, viewOrigin + viewDirection * (probesDistanceMax * 2.0f), viewOrigin - probesDistance, viewOrigin + probesDistance);
    const float viewOriginOffset = viewRayHit.Y * probesDistanceMax * 0.8f;
    viewOrigin += viewDirection * viewOriginOffset;
    const float viewOriginSnapping = giResolution;
    viewOrigin = Vector3::Floor(viewOrigin / viewOriginSnapping) * viewOriginSnapping;
    CalculateVolumeScrolling(ddgiData, viewOrigin);

    // Upload constants
    {
        ddgiData.Result.Constants.ProbesOrigin = ddgiData.ProbesOrigin;
        ddgiData.Result.Constants.ProbesSpacing = ddgiData.ProbesSpacing;
        Quaternion& raysRotation = *(Quaternion*)&ddgiData.Result.Constants.RaysRotation;
        Quaternion::RotationMatrix(raysRotationMatrix, raysRotation);
        raysRotation.Conjugate();
        ddgiData.Result.Constants.ProbesCounts[0] = probesCounts.X;
        ddgiData.Result.Constants.ProbesCounts[1] = probesCounts.Y;
        ddgiData.Result.Constants.ProbesCounts[2] = probesCounts.Z;
        ddgiData.Result.Constants.ProbesScrollOffsets = ddgiData.ProbeScrollOffsets;
        ddgiData.Result.Constants.ProbeScrollDirections = ddgiData.ProbeScrollDirections;
        ddgiData.Result.Constants.ProbeScrollClear[0] = ddgiData.ProbeScrollClear[0] != 0;
        ddgiData.Result.Constants.ProbeScrollClear[1] = ddgiData.ProbeScrollClear[1] != 0;
        ddgiData.Result.Constants.ProbeScrollClear[2] = ddgiData.ProbeScrollClear[2] != 0;
        ddgiData.Result.Constants.RayMaxDistance = 10000.0f; // TODO: adjust to match perf/quality ratio (make it based on Global SDF and Global Surface Atlas distance)
        ddgiData.Result.Constants.ViewDir = viewDirection;
        ddgiData.Result.Constants.RaysCount = probeRaysCount;
        ddgiData.Result.Constants.ProbeHistoryWeight = probeHistoryWeight;
        ddgiData.Result.Constants.IrradianceGamma = 5.0f;
        ddgiData.Result.ProbesState = ddgiData.ProbesState->View();
        ddgiData.Result.ProbesDistance = ddgiData.ProbesDistance->View();
        ddgiData.Result.ProbesIrradiance = ddgiData.ProbesIrradiance->View();

        Data0 data;
        data.DDGI = ddgiData.Result.Constants;
        data.GlobalSDF = bindingDataSDF.Constants;
        data.GlobalSurfaceAtlas = bindingDataSurfaceAtlas.Constants;
        data.IndirectLightingIntensity = indirectLightingIntensity;
        GBufferPass::SetInputs(renderContext.View, data.GBuffer);
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }

    // Classify probes (activation/deactivation and relocation)
    {
        PROFILE_GPU_CPU("Probes Classification");
        uint32 threadGroups = Math::DivideAndRoundUp(probesCount, DDGI_PROBE_CLASSIFY_GROUP_SIZE);
        for (int32 i = 0; i < 4; i++)
        {
            context->BindSR(i, bindingDataSDF.Cascades[i]->ViewVolume());
        }
        context->BindUA(0, ddgiData.Result.ProbesState);
        context->Dispatch(_csClassify, threadGroups, 1, 1);
        context->ResetUA();
    }

    // Trace rays from probes
    {
        PROFILE_GPU_CPU("Trace Rays");

        // Global SDF with Global Surface Atlas software raytracing (X - per probe ray, Y - per probe)
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
        context->Dispatch(_csTraceRays, probeRaysCount / DDGI_TRACE_RAYS_GROUP_SIZE_X, probesCount, 1);
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

    // Update probes
    {
        PROFILE_GPU_CPU("Update Probes");
        context->BindSR(0, ddgiData.Result.ProbesState);
        context->BindSR(1, ddgiData.ProbesTrace->View());

        // Update irradiance
        context->BindUA(0, ddgiData.Result.ProbesIrradiance);
        context->Dispatch(_csUpdateProbesIrradiance, probesCountX, probesCountY, 1);
        uint32 threadGroupsX = Math::DivideAndRoundUp(probesCountX * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        uint32 threadGroupsY = Math::DivideAndRoundUp(probesCountY, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        context->Dispatch(_csUpdateBordersIrradianceRow, threadGroupsX, threadGroupsY, 1);
        threadGroupsX = Math::DivideAndRoundUp(probesCountX, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        threadGroupsY = Math::DivideAndRoundUp(probesCountY * (DDGI_PROBE_RESOLUTION_IRRADIANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        context->Dispatch(_csUpdateBordersIrradianceCollumn, threadGroupsX, threadGroupsY, 1);

        // Update distance
        context->BindUA(0, ddgiData.Result.ProbesDistance);
        context->Dispatch(_csUpdateProbesDistance, probesCountX, probesCountY, 1);
        threadGroupsX = Math::DivideAndRoundUp(probesCountX * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        threadGroupsY = Math::DivideAndRoundUp(probesCountY, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        context->Dispatch(_csUpdateBordersDistanceRow, threadGroupsX, threadGroupsY, 1);
        threadGroupsX = Math::DivideAndRoundUp(probesCountX, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        threadGroupsY = Math::DivideAndRoundUp(probesCountY * (DDGI_PROBE_RESOLUTION_DISTANCE + 2), DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE);
        context->Dispatch(_csUpdateBordersDistanceCollumn, threadGroupsX, threadGroupsY, 1);
    }

    // Render indirect lighting
    {
        PROFILE_GPU_CPU("Indirect Lighting");
#if 0
        // DDGI indirect lighting debug preview
        context->Clear(lightBuffer, Color::Transparent);
#endif
        context->ResetUA();
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
    if (debugProbes)
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
            for (int32 probeIndex = 0; probeIndex < probesCount; probeIndex++)
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

    return false;
}
