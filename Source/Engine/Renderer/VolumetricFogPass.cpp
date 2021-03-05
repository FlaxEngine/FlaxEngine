// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "VolumetricFogPass.h"
#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/Engine.h"

// Must match shader source
int32 VolumetricFogGridInjectionGroupSize = 4;
int32 VolumetricFogIntegrationGroupSize = 8;

VolumetricFogPass::VolumetricFogPass()
    : _shader(nullptr)
{
}

String VolumetricFogPass::ToString() const
{
    return TEXT("VolumetricFogPass");
}

bool VolumetricFogPass::Init()
{
    const auto& limits = GPUDevice::Instance->Limits;
    _isSupported = limits.HasGeometryShaders && limits.HasVolumeTextureRendering && limits.HasCompute && limits.HasInstancing;

    // Create pipeline states
    _psInjectLight.CreatePipelineStates();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/VolumetricFog"));
    if (_shader == nullptr)
    {
        return true;
    }
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<VolumetricFogPass, &VolumetricFogPass::OnShaderReloading>(this);
#endif

    return false;
}

bool VolumetricFogPass::setupResources()
{
    if (!_shader->IsLoaded())
        return true;
    auto shader = _shader->GetShader();

    // Validate shader constant buffers sizes
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }
    if (shader->GetCB(1)->GetSize() != sizeof(PerLight))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 1, PerLight);
        return true;
    }

    // Cache compute shaders
    _csInitialize = shader->GetCS("CS_Initialize");
    _csLightScattering.Get(shader, "CS_LightScattering");
    _csFinalIntegration = shader->GetCS("CS_FinalIntegration");

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psInjectLight.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.VS = shader->GetVS("VS_WriteToSlice");
        psDesc.GS = shader->GetGS("GS_WriteToSlice");
        if (_psInjectLight.Create(psDesc, shader, "PS_InjectLight"))
            return true;
    }

    return false;
}

void VolumetricFogPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psInjectLight.Delete();
    _csInitialize = nullptr;
    _csLightScattering.Clear();
    _csFinalIntegration = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_vbCircleRasterize);
    SAFE_DELETE_GPU_RESOURCE(_ibCircleRasterize);
    _shader = nullptr;
}

float ComputeZSliceFromDepth(float sceneDepth, const VolumetricFogOptions& options, int32 gridSizeZ)
{
    return sceneDepth / options.Distance * (float)gridSizeZ;
}

bool VolumetricFogPass::Init(RenderContext& renderContext, GPUContext* context, VolumetricFogOptions& options)
{
    auto& view = renderContext.View;
    const auto fog = renderContext.List->Fog;

    // Check if already prepared for this frame
    if (renderContext.Buffers->LastFrameVolumetricFog == Engine::FrameCount)
    {
        fog->GetVolumetricFogOptions(options);
        return false;
    }

    // Check if skip rendering
    if (fog == nullptr || (view.Flags & ViewFlags::Fog) == 0 || !_isSupported || checkIfSkipPass())
    {
        RenderTargetPool::Release(renderContext.Buffers->VolumetricFog);
        renderContext.Buffers->VolumetricFog = nullptr;
        renderContext.Buffers->LastFrameVolumetricFog = 0;
        return true;
    }
    fog->GetVolumetricFogOptions(options);
    if (!options.UseVolumetricFog())
    {
        RenderTargetPool::Release(renderContext.Buffers->VolumetricFog);
        renderContext.Buffers->VolumetricFog = nullptr;
        renderContext.Buffers->LastFrameVolumetricFog = 0;
        return true;
    }

    // Setup configuration
    _cache.HistoryWeight = 0.9f;
    _cache.InverseSquaredLightDistanceBiasScale = 1.0f;
    const auto quality = Graphics::VolumetricFogQuality;
    switch (quality)
    {
    case Quality::Low:
    {
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 64;
        _cache.FogJitter = false;
        _cache.TemporalReprojection = false;
        _cache.MissedHistorySamplesCount = 1;
        break;
    }
    case Quality::Medium:
    {
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 64;
        _cache.FogJitter = true;
        _cache.TemporalReprojection = true;
        _cache.MissedHistorySamplesCount = 4;
        break;
    }
    case Quality::High:
    {
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 128;
        _cache.FogJitter = true;
        _cache.TemporalReprojection = true;
        _cache.MissedHistorySamplesCount = 4;
        break;
    }
    case Quality::Ultra:
    {
        _cache.GridPixelSize = 8;
        _cache.GridSizeZ = 256;
        _cache.FogJitter = true;
        _cache.TemporalReprojection = true;
        _cache.MissedHistorySamplesCount = 8;
        break;
    }
    }

    // Prepare
    const int32 width = renderContext.Buffers->GetWidth();
    const int32 height = renderContext.Buffers->GetHeight();
    _cache.GridSize = Vector3(
        (float)Math::DivideAndRoundUp(width, _cache.GridPixelSize),
        (float)Math::DivideAndRoundUp(height, _cache.GridPixelSize),
        (float)_cache.GridSizeZ);
    auto& fogData = renderContext.Buffers->VolumetricFogData;
    fogData.MaxDistance = options.Distance;

    // Init data (partial, without directional light or sky light data);
    GBufferPass::SetInputs(renderContext.View, _cache.Data.GBuffer);
    _cache.Data.GlobalAlbedo = options.Albedo.ToVector3() * options.Albedo.A;
    _cache.Data.GlobalExtinctionScale = options.ExtinctionScale;
    _cache.Data.GlobalEmissive = options.Emissive.ToVector3() * options.Emissive.A;
    _cache.Data.GridSize = _cache.GridSize;
    _cache.Data.GridSizeIntX = (int32)_cache.GridSize.X;
    _cache.Data.GridSizeIntY = (int32)_cache.GridSize.Y;
    _cache.Data.GridSizeIntZ = (int32)_cache.GridSize.Z;
    _cache.Data.HistoryWeight = _cache.HistoryWeight;
    _cache.Data.FogParameters = options.FogParameters;
    _cache.Data.InverseSquaredLightDistanceBiasScale = _cache.InverseSquaredLightDistanceBiasScale;
    _cache.Data.PhaseG = options.ScatteringDistribution;
    _cache.Data.VolumetricFogMaxDistance = options.Distance;
    _cache.Data.MissedHistorySamplesCount = Math::Clamp(_cache.MissedHistorySamplesCount, 1, (int32)ARRAY_COUNT(_cache.Data.FrameJitterOffsets));
    Matrix::Transpose(view.PrevViewProjection, _cache.Data.PrevWorldToClip);
    _cache.Data.DirectionalLightShadow.NumCascades = 0;
    _cache.Data.SkyLight.VolumetricScatteringIntensity = 0;

    // Fill frame jitter history
    const Vector4 defaultOffset = Vector4(0.5f, 0.5f, 0.5f, 0.0f);
    for (int32 i = 0; i < ARRAY_COUNT(_cache.Data.FrameJitterOffsets); i++)
    {
        _cache.Data.FrameJitterOffsets[i] = defaultOffset;
    }
    if (_cache.FogJitter && _cache.TemporalReprojection)
    {
        for (int32 i = 0; i < _cache.MissedHistorySamplesCount; i++)
        {
            const uint64 frameNumber = renderContext.Task->LastUsedFrame - i;
            _cache.Data.FrameJitterOffsets[i] = Vector4(
                RendererUtils::TemporalHalton(frameNumber & 1023, 2),
                RendererUtils::TemporalHalton(frameNumber & 1023, 3),
                RendererUtils::TemporalHalton(frameNumber & 1023, 5),
                0);
        }
    }

    // Set constant buffer data
    auto cb0 = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb0, &_cache.Data);

    // Clear local lights scattering table if was used and will be probably reused later
    if (renderContext.Buffers->LocalShadowedLightScattering)
    {
        if (Vector3::NearEqual(renderContext.Buffers->LocalShadowedLightScattering->Size3(), _cache.GridSize))
        {
            context->Clear(renderContext.Buffers->LocalShadowedLightScattering->ViewVolume(), Color::Transparent);
        }
        else
        {
            RenderTargetPool::Release(renderContext.Buffers->LocalShadowedLightScattering);
            renderContext.Buffers->LocalShadowedLightScattering = nullptr;
        }
    }

    // Render fog this frame
    renderContext.Buffers->LastFrameVolumetricFog = Engine::FrameCount;
    return false;
}

GPUTextureView* VolumetricFogPass::GetLocalShadowedLightScattering(RenderContext& renderContext, GPUContext* context, VolumetricFogOptions& options) const
{
    if (renderContext.Buffers->LocalShadowedLightScattering == nullptr)
    {
        ASSERT(renderContext.Buffers->LastFrameVolumetricFog == Engine::FrameCount);
        const GPUTextureDescription volumeDescRGB = GPUTextureDescription::New3D(_cache.GridSize, PixelFormat::R11G11B10_Float, GPUTextureFlags::RenderTarget | GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
        const auto texture = RenderTargetPool::Get(volumeDescRGB);
        renderContext.Buffers->LocalShadowedLightScattering = texture;
        context->Clear(texture->ViewVolume(), Color::Transparent);
    }

    return renderContext.Buffers->LocalShadowedLightScattering->ViewVolume();
}

template<typename T>
void VolumetricFogPass::RenderRadialLight(RenderContext& renderContext, GPUContext* context, T& light, LightShadowData& shadow)
{
    // Prepare
    VolumetricFogOptions options;
    if (Init(renderContext, context, options))
        return;
    auto& view = renderContext.View;

    // Calculate light volume bounds in camera frustum depth range (min and max)
    BoundingSphere bounds(light.Position, light.Radius);
    Vector3 viewSpaceLightBoundsOrigin = Vector3::Transform(bounds.Center, view.View);
    float furthestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z + bounds.Radius, options, _cache.GridSizeZ);
    float closestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z - bounds.Radius, options, _cache.GridSizeZ);
    int32 volumeZBoundsMin = (int32)Math::Clamp(closestSliceIndexUnclamped, 0.0f, _cache.GridSize.Z - 1.0f);
    int32 volumeZBoundsMax = (int32)Math::Clamp(furthestSliceIndexUnclamped, 0.0f, _cache.GridSize.Z - 1.0f);

    // Cull light
    if ((view.Position - bounds.Center).LengthSquared() >= (options.Distance + bounds.Radius) * (options.Distance + bounds.Radius) || volumeZBoundsMin >= volumeZBoundsMax)
        return;

    PROFILE_GPU_CPU("Volumetric Fog Light");

    // Allocate temporary buffer for light scattering injection
    auto localShadowedLightScattering = GetLocalShadowedLightScattering(renderContext, context, options);

    // Prepare
    PerLight perLight;
    auto cb0 = _shader->GetShader()->GetCB(0);
    auto cb1 = _shader->GetShader()->GetCB(1);

    // Bind the output
    context->SetRenderTarget(localShadowedLightScattering);
    context->SetViewportAndScissors(_cache.Data.GridSize.X, _cache.Data.GridSize.Y);

    // Setup data
    perLight.MinZ = volumeZBoundsMin;
    perLight.LocalLightScatteringIntensity = light.VolumetricScatteringIntensity;
    perLight.ViewSpaceBoundingSphere = Vector4(viewSpaceLightBoundsOrigin, bounds.Radius);
    Matrix::Transpose(view.Projection, perLight.ViewToVolumeClip);
    light.SetupLightData(&perLight.LocalLight, view, true);
    perLight.LocalLightShadow = shadow;

    // Upload data
    context->UpdateCB(cb1, &perLight);
    context->BindCB(0, cb0);
    context->BindCB(1, cb1);

    // Ensure to have valid buffers created
    if (_vbCircleRasterize == nullptr || _ibCircleRasterize == nullptr)
        InitCircleBuffer();

    // Call rendering to the volume
    const int32 psIndex = (_cache.TemporalReprojection ? 1 : 0) + 2;
    context->SetState(_psInjectLight.Get(psIndex));
    const int32 instanceCount = volumeZBoundsMax - volumeZBoundsMin;
    const int32 indexCount = _ibCircleRasterize->GetElementsCount();
    ASSERT(instanceCount > 0);
    context->BindVB(ToSpan(&_vbCircleRasterize, 1));
    context->BindIB(_ibCircleRasterize);
    context->DrawIndexedInstanced(indexCount, instanceCount, 0);

    // Cleanup
    context->UnBindCB(0);
    context->UnBindCB(1);
    auto viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
    context->ResetRenderTarget();
    context->FlushState();

    // Mark as rendered
    light.RenderedVolumetricFog = 1;
}

template<typename T>
void VolumetricFogPass::RenderRadialLight(RenderContext& renderContext, GPUContext* context, RenderView& view, VolumetricFogOptions& options, T& light, PerLight& perLight, GPUConstantBuffer* cb1)
{
    const BoundingSphere bounds(light.Position, light.Radius);
    auto& cache = _cache;

    // Calculate light volume bounds in camera frustum depth range (min and max)
    const Vector3 viewSpaceLightBoundsOrigin = Vector3::Transform(bounds.Center, view.View);
    const float furthestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z + bounds.Radius, options, cache.GridSizeZ);
    const float closestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z - bounds.Radius, options, cache.GridSizeZ);
    const int32 volumeZBoundsMin = (int32)Math::Clamp(closestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);
    const int32 volumeZBoundsMax = (int32)Math::Clamp(furthestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);

    if (volumeZBoundsMin < volumeZBoundsMax)
    {
        // TODO: use full scene shadows atlas and render point/spot lights with shadow into a fog volume
        bool withShadow = false;

        // Setup data
        perLight.MinZ = volumeZBoundsMin;
        perLight.LocalLightScatteringIntensity = light.VolumetricScatteringIntensity;
        perLight.ViewSpaceBoundingSphere = Vector4(viewSpaceLightBoundsOrigin, bounds.Radius);
        Matrix::Transpose(renderContext.View.Projection, perLight.ViewToVolumeClip);
        light.SetupLightData(&perLight.LocalLight, renderContext.View, withShadow);

        // Upload data
        context->UpdateCB(cb1, &perLight);
        context->BindCB(1, cb1);

        // Ensure to have valid buffers created
        if (_vbCircleRasterize == nullptr || _ibCircleRasterize == nullptr)
            InitCircleBuffer();

        // Call rendering to the volume
        const int32 psIndex = (cache.TemporalReprojection ? 1 : 0) + (withShadow ? 2 : 0);
        context->SetState(_psInjectLight.Get(psIndex));
        const int32 instanceCount = volumeZBoundsMax - volumeZBoundsMin;
        const int32 indexCount = _ibCircleRasterize->GetElementsCount();
        context->BindVB(ToSpan(&_vbCircleRasterize, 1));
        context->BindIB(_ibCircleRasterize);
        context->DrawIndexedInstanced(indexCount, instanceCount, 0);
    }
}

void VolumetricFogPass::RenderLight(RenderContext& renderContext, GPUContext* context, RendererPointLightData& light, GPUTextureView* shadowMap, LightShadowData& shadow)
{
    // Skip lights with no volumetric light influence or not casting volumetric shadow
    if (light.VolumetricScatteringIntensity <= ZeroTolerance || !light.CastVolumetricShadow)
        return;
    ASSERT(shadowMap);

    context->BindSR(5, shadowMap);

    RenderRadialLight(renderContext, context, light, shadow);

    context->UnBindSR(5);
}

void VolumetricFogPass::RenderLight(RenderContext& renderContext, GPUContext* context, RendererSpotLightData& light, GPUTextureView* shadowMap, LightShadowData& shadow)
{
    // Skip lights with no volumetric light influence or not casting volumetric shadow
    if (light.VolumetricScatteringIntensity <= ZeroTolerance || !light.CastVolumetricShadow)
        return;
    ASSERT(shadowMap);

    context->BindSR(6, shadowMap);

    RenderRadialLight(renderContext, context, light, shadow);

    context->UnBindSR(6);
}

void VolumetricFogPass::Render(RenderContext& renderContext)
{
    // Prepare
    VolumetricFogOptions options;
    auto context = GPUDevice::Instance->GetMainContext();
    if (Init(renderContext, context, options))
        return;
    auto& view = renderContext.View;
    auto& cache = _cache;

    PROFILE_GPU_CPU("Volumetric Fog");

    // TODO: test exponential depth distribution (should give better quality near the camera)
    // TODO: use tiled light culling and render unshadowed lights in single pass

    // Init directional light data
    GPUTextureView* dirLightShadowMap = nullptr;
    if (renderContext.List->DirectionalLights.HasItems())
    {
        const int32 dirLightIndex = (int32)renderContext.List->DirectionalLights.Count() - 1;
        const auto& dirLight = renderContext.List->DirectionalLights[dirLightIndex];
        const float brightness = dirLight.VolumetricScatteringIntensity;

        if (brightness > ZeroTolerance)
        {
            const auto shadowPass = ShadowsPass::Instance();
            const bool useShadow = dirLight.CastVolumetricShadow && shadowPass->LastDirLightIndex == dirLightIndex;
            dirLight.SetupLightData(&_cache.Data.DirectionalLight, view, useShadow);
            _cache.Data.DirectionalLight.Color *= brightness;
            if (useShadow)
            {
                _cache.Data.DirectionalLightShadow = shadowPass->LastDirLight;
                dirLightShadowMap = shadowPass->LastDirLightShadowMap;
            }
            else
            {
                _cache.Data.DirectionalLightShadow.NumCascades = 0;
            }
        }
    }

    // Init sky light data
    GPUTexture* skyLightImage = nullptr;
    if (renderContext.List->SkyLights.HasItems())
    {
        const auto& skyLight = renderContext.List->SkyLights.Last();

        if (skyLight.VolumetricScatteringIntensity > ZeroTolerance)
        {
            _cache.Data.SkyLight.MultiplyColor = skyLight.Color;
            _cache.Data.SkyLight.AdditiveColor = skyLight.AdditiveColor;
            _cache.Data.SkyLight.VolumetricScatteringIntensity = skyLight.VolumetricScatteringIntensity;
            const auto source = skyLight.Image;
            skyLightImage = source ? source->GetTexture() : nullptr;
        }
    }

    // Set constant buffer data
    auto cb0 = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb0, &_cache.Data);
    context->BindCB(0, cb0);

    // Peek flags
    const bool temporalHistoryIsValid = cache.TemporalReprojection
            && renderContext.Buffers->VolumetricFogHistory
            && !renderContext.Task->IsCameraCut
            && Vector3::NearEqual(renderContext.Buffers->VolumetricFogHistory->Size3(), cache.GridSize);

    // Allocate buffers
    const GPUTextureDescription volumeDesc = GPUTextureDescription::New3D(cache.GridSize, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::RenderTarget | GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
    const GPUTextureDescription volumeDescRGB = GPUTextureDescription::New3D(cache.GridSize, PixelFormat::R11G11B10_Float, GPUTextureFlags::RenderTarget | GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
    auto vBufferA = RenderTargetPool::Get(volumeDesc);
    auto vBufferB = RenderTargetPool::Get(volumeDescRGB);
    const auto lightScattering = RenderTargetPool::Get(volumeDesc);

    int32 groupCountX = Math::DivideAndRoundUp((int32)cache.GridSize.X, VolumetricFogGridInjectionGroupSize);
    int32 groupCountY = Math::DivideAndRoundUp((int32)cache.GridSize.Y, VolumetricFogGridInjectionGroupSize);
    int32 groupCountZ = Math::DivideAndRoundUp((int32)cache.GridSize.Z, VolumetricFogGridInjectionGroupSize);

    // Initialize fog volume properties
    {
        PROFILE_GPU("Initialize");

        context->ResetRenderTarget();
        context->BindUA(0, vBufferA->ViewVolume());
        context->BindUA(1, vBufferB->ViewVolume());

        context->Dispatch(_csInitialize, groupCountX, groupCountY, groupCountZ);

        context->UnBindUA(0);
        context->UnBindUA(1);
        context->FlushState();
    }

    // Render local fog particles
    if (renderContext.List->VolumetricFogParticles.HasItems())
    {
        PROFILE_GPU_CPU("Local Fog");

        // Bind the output
        GPUTextureView* rt[] = { vBufferA->ViewVolume(), vBufferB->ViewVolume() };
        context->SetRenderTarget(nullptr, Span<GPUTextureView*>(rt, 2));
        context->SetViewportAndScissors((float)volumeDesc.Width, (float)volumeDesc.Height);

        // Ensure to have valid buffers created
        if (_vbCircleRasterize == nullptr || _ibCircleRasterize == nullptr)
            InitCircleBuffer();

        MaterialBase::BindParameters bindParams(context, renderContext);
        bindParams.DrawCallsCount = 1;
        CustomData customData;
        customData.Shader = _shader->GetShader();
        customData.GridSize = cache.GridSize;
        customData.VolumetricFogMaxDistance = cache.Data.VolumetricFogMaxDistance;
        bindParams.CustomData = &customData;

        for (auto& drawCall : renderContext.List->VolumetricFogParticles)
        {
            const BoundingSphere bounds(drawCall.Particle.VolumetricFog.Position, drawCall.Particle.VolumetricFog.Radius);
            ASSERT(!bounds.Center.IsNanOrInfinity() && !isnan(bounds.Radius) && !isinf(bounds.Radius));

            // Calculate light volume bounds in camera frustum depth range (min and max)
            const Vector3 viewSpaceLightBoundsOrigin = Vector3::Transform(bounds.Center, view.View);
            const float furthestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z + bounds.Radius, options, cache.GridSizeZ);
            const float closestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z - bounds.Radius, options, cache.GridSizeZ);
            const int32 volumeZBoundsMin = (int32)Math::Clamp(closestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);
            const int32 volumeZBoundsMax = (int32)Math::Clamp(furthestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);

            // Culling
            if ((view.Position - bounds.Center).LengthSquared() >= (options.Distance + bounds.Radius) * (options.Distance + bounds.Radius) || volumeZBoundsMin >= volumeZBoundsMax)
                continue;

            // Setup material shader data
            customData.ParticleIndex = drawCall.Particle.VolumetricFog.ParticleIndex;
            bindParams.FirstDrawCall = &drawCall;
            drawCall.Material->Bind(bindParams);

            // Setup volumetric shader data
            PerLight perLight;
            auto cb1 = _shader->GetShader()->GetCB(1);
            perLight.SliceToDepth.X = cache.Data.GridSize.Z;
            perLight.SliceToDepth.Y = cache.Data.VolumetricFogMaxDistance;
            perLight.MinZ = volumeZBoundsMin;
            perLight.ViewSpaceBoundingSphere = Vector4(viewSpaceLightBoundsOrigin, bounds.Radius);
            Matrix::Transpose(renderContext.View.Projection, perLight.ViewToVolumeClip);

            // Upload data
            context->UpdateCB(cb1, &perLight);
            context->BindCB(1, cb1);

            // Call rendering to the volume
            const int32 instanceCount = volumeZBoundsMax - volumeZBoundsMin;
            const int32 indexCount = _ibCircleRasterize->GetElementsCount();
            context->BindVB(ToSpan(&_vbCircleRasterize, 1));
            context->BindIB(_ibCircleRasterize);
            context->DrawIndexedInstanced(indexCount, instanceCount, 0);
        }

        context->ResetRenderTarget();
    }

    // Render Lights
    GPUTextureView* localShadowedLightScattering = nullptr;
    {
        // Get lights to render
        Array<const RendererPointLightData*> pointLights;
        Array<const RendererSpotLightData*> spotLights;
        for (int32 i = 0; i < renderContext.List->PointLights.Count(); i++)
        {
            const auto& light = renderContext.List->PointLights[i];
            if (light.VolumetricScatteringIntensity > ZeroTolerance && !light.RenderedVolumetricFog)
            {
                if ((view.Position - light.Position).LengthSquared() < (options.Distance + light.Radius) * (options.Distance + light.Radius))
                {
                    pointLights.Add(&light);
                }
            }
        }
        for (int32 i = 0; i < renderContext.List->SpotLights.Count(); i++)
        {
            const auto& light = renderContext.List->SpotLights[i];
            if (light.VolumetricScatteringIntensity > ZeroTolerance && !light.RenderedVolumetricFog)
            {
                if ((view.Position - light.Position).LengthSquared() < (options.Distance + light.Radius) * (options.Distance + light.Radius))
                {
                    spotLights.Add(&light);
                }
            }
        }

        // Skip if no lights to render
        if (pointLights.Count() + spotLights.Count())
        {
            PROFILE_GPU_CPU("Lights Injection");

            // Allocate temporary buffer for light scattering injection
            localShadowedLightScattering = GetLocalShadowedLightScattering(renderContext, context, options);

            // Prepare
            PerLight perLight;
            perLight.SliceToDepth.X = cache.Data.GridSize.Z;
            perLight.SliceToDepth.Y = cache.Data.VolumetricFogMaxDistance;
            auto cb1 = _shader->GetShader()->GetCB(1);

            // Bind the output
            context->SetRenderTarget(localShadowedLightScattering);
            context->SetViewportAndScissors((float)volumeDesc.Width, (float)volumeDesc.Height);

            // Render them to the volume
            for (int32 i = 0; i < pointLights.Count(); i++)
                RenderRadialLight(renderContext, context, view, options, *pointLights[i], perLight, cb1);
            for (int32 i = 0; i < spotLights.Count(); i++)
                RenderRadialLight(renderContext, context, view, options, *spotLights[i], perLight, cb1);

            // Cleanup
            context->UnBindCB(1);
            context->ResetRenderTarget();
            context->FlushState();
        }
        else if (renderContext.Buffers->LocalShadowedLightScattering)
        {
            localShadowedLightScattering = renderContext.Buffers->LocalShadowedLightScattering->ViewVolume();
        }
    }

    // Light Scattering
    {
        PROFILE_GPU("Light Scattering");

        const auto lightScatteringHistory = temporalHistoryIsValid ? renderContext.Buffers->VolumetricFogHistory : nullptr;

        context->BindUA(0, lightScattering->ViewVolume());
        context->BindSR(0, vBufferA->ViewVolume());
        context->BindSR(1, vBufferB->ViewVolume());
        context->BindSR(2, lightScatteringHistory ? lightScatteringHistory->ViewVolume() : nullptr);
        context->BindSR(3, localShadowedLightScattering);
        context->BindSR(4, dirLightShadowMap);
        context->BindSR(5, skyLightImage);

        const int32 csIndex = cache.TemporalReprojection ? 1 : 0;
        context->Dispatch(_csLightScattering.Get(csIndex), groupCountX, groupCountY, groupCountZ);
    }

    // Release resources
    RenderTargetPool::Release(vBufferA);
    RenderTargetPool::Release(vBufferB);

    // Update the temporal history buffer
    if (renderContext.Buffers->VolumetricFogHistory)
    {
        RenderTargetPool::Release(renderContext.Buffers->VolumetricFogHistory);
    }
    renderContext.Buffers->VolumetricFogHistory = lightScattering;

    // Get buffer for the integrated light scattering (try to reuse the previous frame if it's valid)
    GPUTexture* integratedLightScattering = renderContext.Buffers->VolumetricFog;
    if (integratedLightScattering == nullptr || !Vector3::NearEqual(integratedLightScattering->Size3(), cache.GridSize))
    {
        if (integratedLightScattering)
        {
            RenderTargetPool::Release(integratedLightScattering);
        }
        integratedLightScattering = RenderTargetPool::Get(volumeDesc);
        renderContext.Buffers->VolumetricFog = integratedLightScattering;
    }
    renderContext.Buffers->LastFrameVolumetricFog = Engine::FrameCount;

    groupCountX = Math::DivideAndRoundUp((int32)cache.GridSize.X, VolumetricFogIntegrationGroupSize);
    groupCountY = Math::DivideAndRoundUp((int32)cache.GridSize.Y, VolumetricFogIntegrationGroupSize);

    // Final Integration
    {
        PROFILE_GPU("Final Integration");

        context->ResetSR();
        context->BindUA(0, integratedLightScattering->ViewVolume());
        context->FlushState();
        context->BindSR(0, lightScattering->ViewVolume());

        context->Dispatch(_csFinalIntegration, groupCountX, groupCountY, 1);
    }

    // Cleanup
    context->UnBindUA(0);
    context->ResetRenderTarget();
    auto viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
    context->FlushState();
}

void VolumetricFogPass::InitCircleBuffer()
{
    const int32 vertices = 8;
    const int32 triangles = vertices - 2;
    const int32 rings = vertices;
    const float radiansPerRingSegment = PI / (float)rings;
    Vector2 vbData[vertices];
    uint16 ibData[triangles * 3];

    const float radiusScale = 1.0f / Math::Cos(radiansPerRingSegment);
    for (int32 vertexIndex = 0; vertexIndex < vertices; vertexIndex++)
    {
        const float angle = vertexIndex / static_cast<float>(vertices - 1) * 2 * PI;
        vbData[vertexIndex] = Vector2(radiusScale * Math::Cos(angle) * 0.5f + 0.5f, radiusScale * Math::Sin(angle) * 0.5f + 0.5f);
    }
    int32 ibIndex = 0;
    for (int32 triangleIndex = 0; triangleIndex < triangles; triangleIndex++)
    {
        const int32 firstVertexIndex = triangleIndex + 2;
        ibData[ibIndex++] = 0;
        ibData[ibIndex++] = firstVertexIndex - 1;
        ibData[ibIndex++] = firstVertexIndex;
    }

    // Create buffers
    ASSERT(_vbCircleRasterize == nullptr && _ibCircleRasterize == nullptr);
    _vbCircleRasterize = GPUDevice::Instance->CreateBuffer(TEXT("VolumetricFog.CircleRasterize.VB"));
    _ibCircleRasterize = GPUDevice::Instance->CreateBuffer(TEXT("VolumetricFog.CircleRasterize.IB"));
    if (_vbCircleRasterize->Init(GPUBufferDescription::Vertex(sizeof(Vector2), vertices, vbData))
        || _ibCircleRasterize->Init(GPUBufferDescription::Index(sizeof(uint16), triangles * 3, ibData)))
    {
        LOG(Fatal, "Failed to setup volumetric fog buffers.");
    }
}
