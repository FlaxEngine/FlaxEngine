// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "VolumetricFogPass.h"
#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/GPUContext.h"
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
    // CB1 is used for per-draw info (ObjectIndex)
    if (shader->GetCB(2)->GetSize() != sizeof(PerLight))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 2, PerLight);
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
        if (fog)
            fog->GetVolumetricFogOptions(options);
        return false;
    }

    // Check if skip rendering
    if (fog == nullptr || (view.Flags & ViewFlags::Fog) == ViewFlags::None || !_isSupported || checkIfSkipPass())
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
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 64;
        _cache.FogJitter = false;
        _cache.MissedHistorySamplesCount = 1;
        break;
    case Quality::Medium:
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 64;
        _cache.FogJitter = true;
        _cache.MissedHistorySamplesCount = 4;
        break;
    case Quality::High:
        _cache.GridPixelSize = 16;
        _cache.GridSizeZ = 128;
        _cache.FogJitter = true;
        _cache.MissedHistorySamplesCount = 4;
        break;
    case Quality::Ultra:
        _cache.GridPixelSize = 8;
        _cache.GridSizeZ = 256;
        _cache.FogJitter = true;
        _cache.MissedHistorySamplesCount = 8;
        break;
    }

    // Prepare
    const int32 width = renderContext.Buffers->GetWidth();
    const int32 height = renderContext.Buffers->GetHeight();
    _cache.GridSize = Float3(
        (float)Math::DivideAndRoundUp(width, _cache.GridPixelSize),
        (float)Math::DivideAndRoundUp(height, _cache.GridPixelSize),
        (float)_cache.GridSizeZ);
    auto& fogData = renderContext.Buffers->VolumetricFogData;
    fogData.MaxDistance = options.Distance;

    // Init data (partial, without directional light or sky light data);
    GBufferPass::SetInputs(renderContext.View, _cache.Data.GBuffer);
    _cache.Data.GlobalAlbedo = options.Albedo.ToFloat3() * options.Albedo.A;
    _cache.Data.GlobalExtinctionScale = options.ExtinctionScale;
    _cache.Data.GlobalEmissive = options.Emissive.ToFloat3() * options.Emissive.A;
    _cache.Data.GridSize = _cache.GridSize;
    _cache.Data.GridSizeIntX = (uint32)_cache.GridSize.X;
    _cache.Data.GridSizeIntY = (uint32)_cache.GridSize.Y;
    _cache.Data.GridSizeIntZ = (uint32)_cache.GridSize.Z;
    _cache.Data.HistoryWeight = _cache.HistoryWeight;
    _cache.Data.FogParameters = options.FogParameters;
    _cache.Data.InverseSquaredLightDistanceBiasScale = _cache.InverseSquaredLightDistanceBiasScale;
    _cache.Data.PhaseG = options.ScatteringDistribution;
    _cache.Data.VolumetricFogMaxDistance = options.Distance;
    _cache.Data.MissedHistorySamplesCount = Math::Clamp(_cache.MissedHistorySamplesCount, 1, (int32)ARRAY_COUNT(_cache.Data.FrameJitterOffsets));
    Matrix::Transpose(view.PrevViewProjection, _cache.Data.PrevWorldToClip);
    _cache.Data.SkyLight.VolumetricScatteringIntensity = 0;

    // Fill frame jitter history
    const Float4 defaultOffset(0.5f, 0.5f, 0.5f, 0.0f);
    for (int32 i = 0; i < ARRAY_COUNT(_cache.Data.FrameJitterOffsets); i++)
    {
        _cache.Data.FrameJitterOffsets[i] = defaultOffset;
    }
    if (_cache.FogJitter)
    {
        for (int32 i = 0; i < _cache.MissedHistorySamplesCount; i++)
        {
            const uint64 frameNumber = renderContext.Task->LastUsedFrame - i;
            _cache.Data.FrameJitterOffsets[i] = Float4(
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
        if (Float3::NearEqual(renderContext.Buffers->LocalShadowedLightScattering->Size3(), _cache.GridSize))
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
        RENDER_TARGET_POOL_SET_NAME(texture, "VolumetricFog.LocalShadowedLightScattering");
        renderContext.Buffers->LocalShadowedLightScattering = texture;
        context->Clear(texture->ViewVolume(), Color::Transparent);
    }

    return renderContext.Buffers->LocalShadowedLightScattering->ViewVolume();
}

template<typename T>
void VolumetricFogPass::RenderRadialLight(RenderContext& renderContext, GPUContext* context, RenderView& view, VolumetricFogOptions& options, T& light, PerLight& perLight, GPUConstantBuffer* cb2)
{
    const Float3 center = light.Position;
    const float radius = light.Radius;
    ASSERT(!center.IsNanOrInfinity() && !isnan(radius) && !isinf(radius));
    auto& cache = _cache;

    // Calculate light volume bounds in camera frustum depth range (min and max)
    const Float3 viewSpaceLightBoundsOrigin = Float3::Transform(center, view.View);
    const float furthestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z + radius, options, cache.GridSizeZ);
    const float closestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z - radius, options, cache.GridSizeZ);
    const int32 volumeZBoundsMin = (int32)Math::Clamp(closestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);
    const int32 volumeZBoundsMax = (int32)Math::Clamp(furthestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);
    if (volumeZBoundsMin >= volumeZBoundsMax)
        return;

    // Setup data
    perLight.SliceToDepth.X = cache.Data.GridSize.Z;
    perLight.SliceToDepth.Y = cache.Data.VolumetricFogMaxDistance;
    perLight.MinZ = volumeZBoundsMin;
    perLight.LocalLightScatteringIntensity = light.VolumetricScatteringIntensity;
    perLight.ViewSpaceBoundingSphere = Float4(viewSpaceLightBoundsOrigin, radius);
    Matrix::Transpose(renderContext.View.Projection, perLight.ViewToVolumeClip);
    const bool withShadow = light.CastVolumetricShadow && light.HasShadow;
    light.SetShaderData(perLight.LocalLight, withShadow);

    // Upload data
    context->UpdateCB(cb2, &perLight);
    context->BindCB(2, cb2);

    // Ensure to have valid buffers created
    if (_vbCircleRasterize == nullptr || _ibCircleRasterize == nullptr)
        InitCircleBuffer();

    // Call rendering to the volume
    const int32 psIndex = withShadow ? 1 : 0;
    context->SetState(_psInjectLight.Get(psIndex));
    const int32 instanceCount = volumeZBoundsMax - volumeZBoundsMin;
    const int32 indexCount = _ibCircleRasterize->GetElementsCount();
    context->BindVB(ToSpan(&_vbCircleRasterize, 1));
    context->BindIB(_ibCircleRasterize);
    context->DrawIndexedInstanced(indexCount, instanceCount, 0);
}

void VolumetricFogPass::Render(RenderContext& renderContext)
{
    VolumetricFogOptions options;
    auto context = GPUDevice::Instance->GetMainContext();
    if (Init(renderContext, context, options))
        return;
    auto& view = renderContext.View;
    auto& cache = _cache;
    PROFILE_GPU_CPU("Volumetric Fog");

    // TODO: test exponential depth distribution (should give better quality near the camera)
    // TODO: use tiled light culling and render unshadowed lights in single pass

    // Try to get shadows atlas
    GPUTexture* shadowMap;
    GPUBufferView* shadowsBuffer;
    ShadowsPass::GetShadowAtlas(renderContext.Buffers, shadowMap, shadowsBuffer);

    // Init directional light data
    Platform::MemoryClear(&_cache.Data.DirectionalLight, sizeof(_cache.Data.DirectionalLight));
    if (renderContext.List->DirectionalLights.HasItems())
    {
        const int32 dirLightIndex = (int32)renderContext.List->DirectionalLights.Count() - 1;
        const auto& dirLight = renderContext.List->DirectionalLights[dirLightIndex];
        const float brightness = dirLight.VolumetricScatteringIntensity;
        if (brightness > ZeroTolerance)
        {
            const bool useShadow = shadowMap && dirLight.CastVolumetricShadow && dirLight.HasShadow;
            dirLight.SetShaderData(_cache.Data.DirectionalLight, useShadow);
            _cache.Data.DirectionalLight.Color *= brightness;
        }
    }

    // Init GI data
    bool useDDGI = false;
    DynamicDiffuseGlobalIlluminationPass::BindingData bindingDataDDGI;
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::GI))
    {
        switch (renderContext.List->Settings.GlobalIllumination.Mode)
        {
        case GlobalIlluminationMode::DDGI:
            if (!DynamicDiffuseGlobalIlluminationPass::Instance()->Get(renderContext.Buffers, bindingDataDDGI))
            {
                _cache.Data.DDGI = bindingDataDDGI.Constants;
                useDDGI = true;
            }
            break;
        }
    }

    // Init sky light data
    GPUTexture* skyLightImage = nullptr;
    Platform::MemoryClear(&_cache.Data.SkyLight, sizeof(_cache.Data.SkyLight));
    if (renderContext.List->SkyLights.HasItems() && !useDDGI)
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

    // Allocate buffers
    const GPUTextureDescription volumeDesc = GPUTextureDescription::New3D(cache.GridSize, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::RenderTarget | GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
    const GPUTextureDescription volumeDescRGB = GPUTextureDescription::New3D(cache.GridSize, PixelFormat::R11G11B10_Float, GPUTextureFlags::RenderTarget | GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
    auto vBufferA = RenderTargetPool::Get(volumeDesc);
    RENDER_TARGET_POOL_SET_NAME(vBufferA, "VolumetricFog.VBufferA");
    auto vBufferB = RenderTargetPool::Get(volumeDescRGB);
    RENDER_TARGET_POOL_SET_NAME(vBufferB, "VolumetricFog.VBufferB");
    const auto lightScattering = RenderTargetPool::Get(volumeDesc);
    RENDER_TARGET_POOL_SET_NAME(lightScattering, "VolumetricFog.LightScattering");

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
        context->ResetUA();
    }

    // Render local fog particles
    if (renderContext.List->VolumetricFogParticles.HasItems())
    {
        PROFILE_GPU_CPU_NAMED("Local Fog");

        // Bind the output
        GPUTextureView* rt[] = { vBufferA->ViewVolume(), vBufferB->ViewVolume() };
        context->SetRenderTarget(nullptr, Span<GPUTextureView*>(rt, 2));
        context->SetViewportAndScissors((float)volumeDesc.Width, (float)volumeDesc.Height);

        // Ensure to have valid buffers created
        if (_vbCircleRasterize == nullptr || _ibCircleRasterize == nullptr)
            InitCircleBuffer();

        MaterialBase::BindParameters bindParams(context, renderContext);
        CustomData customData;
        customData.Shader = _shader->GetShader();
        customData.GridSize = cache.GridSize;
        customData.VolumetricFogMaxDistance = cache.Data.VolumetricFogMaxDistance;
        bindParams.CustomData = &customData;
        bindParams.BindViewData();
        bindParams.DrawCall = &renderContext.List->VolumetricFogParticles.First();
        bindParams.BindDrawData();

        for (auto& drawCall : renderContext.List->VolumetricFogParticles)
        {
            const Float3 center = drawCall.Particle.VolumetricFog.Position;
            const float radius = drawCall.Particle.VolumetricFog.Radius;
            ASSERT(!center.IsNanOrInfinity() && !isnan(radius) && !isinf(radius));

            // Calculate light volume bounds in camera frustum depth range (min and max)
            const Float3 viewSpaceLightBoundsOrigin = Float3::Transform(center, view.View);
            const float furthestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z + radius, options, cache.GridSizeZ);
            const float closestSliceIndexUnclamped = ComputeZSliceFromDepth(viewSpaceLightBoundsOrigin.Z - radius, options, cache.GridSizeZ);
            const int32 volumeZBoundsMin = (int32)Math::Clamp(closestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);
            const int32 volumeZBoundsMax = (int32)Math::Clamp(furthestSliceIndexUnclamped, 0.0f, cache.GridSize.Z - 1.0f);

            // Culling
            if ((view.Position - center).LengthSquared() >= Math::Square(options.Distance + radius) || volumeZBoundsMin >= volumeZBoundsMax)
                continue;

            // Setup material shader data
            customData.ParticleIndex = drawCall.Particle.VolumetricFog.ParticleIndex;
            bindParams.DrawCall = &drawCall;
            drawCall.Material->Bind(bindParams);

            // Setup volumetric shader data
            PerLight perLight;
            auto cb2 = _shader->GetShader()->GetCB(2);
            perLight.SliceToDepth.X = cache.Data.GridSize.Z;
            perLight.SliceToDepth.Y = cache.Data.VolumetricFogMaxDistance;
            perLight.MinZ = volumeZBoundsMin;
            perLight.ViewSpaceBoundingSphere = Float4(viewSpaceLightBoundsOrigin, radius);
            Matrix::Transpose(renderContext.View.Projection, perLight.ViewToVolumeClip);

            // Upload data
            context->UpdateCB(cb2, &perLight);
            context->BindCB(2, cb2);

            // Call rendering to the volume
            const int32 instanceCount = volumeZBoundsMax - volumeZBoundsMin;
            const int32 indexCount = _ibCircleRasterize->GetElementsCount();
            context->BindVB(ToSpan(&_vbCircleRasterize, 1));
            context->BindIB(_ibCircleRasterize);
            context->DrawIndexedInstanced(indexCount, instanceCount, 0);
        }

        context->ResetRenderTarget();
        context->BindCB(0, cb0);
    }

    // Render Lights
    GPUTextureView* localShadowedLightScattering = nullptr;
    {
        // Get lights to render
        Array<uint16, InlinedAllocation<64, RendererAllocation>> pointLights;
        Array<uint16, InlinedAllocation<64, RendererAllocation>> spotLights;
        for (int32 i = 0; i < renderContext.List->PointLights.Count(); i++)
        {
            const auto& light = renderContext.List->PointLights.Get()[i];
            if (light.VolumetricScatteringIntensity > ZeroTolerance &&
                (view.Position - light.Position).LengthSquared() < Math::Square(options.Distance + light.Radius))
                pointLights.Add(i);
        }
        for (int32 i = 0; i < renderContext.List->SpotLights.Count(); i++)
        {
            const auto& light = renderContext.List->SpotLights.Get()[i];
            if (light.VolumetricScatteringIntensity > ZeroTolerance &&
                (view.Position - light.Position).LengthSquared() < Math::Square(options.Distance + light.Radius))
                spotLights.Add(i);
        }

        // Skip if no lights to render
        if (pointLights.Count() + spotLights.Count())
        {
            PROFILE_GPU_CPU_NAMED("Lights Injection");

            // Allocate temporary buffer for light scattering injection
            localShadowedLightScattering = GetLocalShadowedLightScattering(renderContext, context, options);

            // Prepare
            PerLight perLight;
            perLight.SliceToDepth.X = cache.Data.GridSize.Z;
            perLight.SliceToDepth.Y = cache.Data.VolumetricFogMaxDistance;
            auto cb2 = _shader->GetShader()->GetCB(2);

            // Bind the output
            context->SetRenderTarget(localShadowedLightScattering);
            context->SetViewportAndScissors((float)volumeDesc.Width, (float)volumeDesc.Height);

            // Render them to the volume
            context->BindSR(0, shadowMap);
            context->BindSR(1, shadowsBuffer);
            for (int32 i = 0; i < pointLights.Count(); i++)
                RenderRadialLight(renderContext, context, view, options, renderContext.List->PointLights[pointLights[i]], perLight, cb2);
            for (int32 i = 0; i < spotLights.Count(); i++)
                RenderRadialLight(renderContext, context, view, options, renderContext.List->SpotLights[spotLights[i]], perLight, cb2);

            // Cleanup
            context->UnBindCB(2);
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

        const bool temporalHistoryIsValid = renderContext.Buffers->VolumetricFogHistory && !renderContext.Task->IsCameraCut && Float3::NearEqual(renderContext.Buffers->VolumetricFogHistory->Size3(), cache.GridSize);
        const auto lightScatteringHistory = temporalHistoryIsValid ? renderContext.Buffers->VolumetricFogHistory : nullptr;

        context->BindUA(0, lightScattering->ViewVolume());
        context->BindSR(0, vBufferA->ViewVolume());
        context->BindSR(1, vBufferB->ViewVolume());
        context->BindSR(2, lightScatteringHistory ? lightScatteringHistory->ViewVolume() : nullptr);
        context->BindSR(3, localShadowedLightScattering);
        context->BindSR(4, shadowMap);
        context->BindSR(5, shadowsBuffer);
        int32 csIndex;
        if (useDDGI)
        {
            context->BindSR(6, bindingDataDDGI.ProbesData);
            context->BindSR(7, bindingDataDDGI.ProbesDistance);
            context->BindSR(8, bindingDataDDGI.ProbesIrradiance);
            csIndex = 1;
        }
        else
        {
            context->BindSR(6, skyLightImage);
            csIndex = 0;
        }
        context->Dispatch(_csLightScattering.Get(csIndex), groupCountX, groupCountY, groupCountZ);

        context->ResetSR();
        context->ResetUA();
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
    if (integratedLightScattering == nullptr || !Float3::NearEqual(integratedLightScattering->Size3(), cache.GridSize))
    {
        if (integratedLightScattering)
        {
            RenderTargetPool::Release(integratedLightScattering);
        }
        integratedLightScattering = RenderTargetPool::Get(volumeDesc);
        RENDER_TARGET_POOL_SET_NAME(integratedLightScattering, "VolumetricFog.Integrated");
        renderContext.Buffers->VolumetricFog = integratedLightScattering;
    }
    renderContext.Buffers->LastFrameVolumetricFog = Engine::FrameCount;

    groupCountX = Math::DivideAndRoundUp((int32)cache.GridSize.X, VolumetricFogIntegrationGroupSize);
    groupCountY = Math::DivideAndRoundUp((int32)cache.GridSize.Y, VolumetricFogIntegrationGroupSize);

    // Final Integration
    {
        PROFILE_GPU("Final Integration");

        context->BindUA(0, integratedLightScattering->ViewVolume());
        context->BindSR(0, lightScattering->ViewVolume());

        context->Dispatch(_csFinalIntegration, groupCountX, groupCountY, 1);
    }

    // Cleanup
    context->ResetUA();
    context->ResetSR();
    context->ResetRenderTarget();
    auto viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
}

void VolumetricFogPass::InitCircleBuffer()
{
    const int32 vertices = 8;
    const int32 triangles = vertices - 2;
    const int32 rings = vertices;
    const float radiansPerRingSegment = PI / (float)rings;
    Float2 vbData[vertices];
    uint16 ibData[triangles * 3];

    const float radiusScale = 1.0f / Math::Cos(radiansPerRingSegment);
    for (int32 vertexIndex = 0; vertexIndex < vertices; vertexIndex++)
    {
        const float angle = vertexIndex / static_cast<float>(vertices - 1) * 2 * PI;
        vbData[vertexIndex] = Float2(radiusScale * Math::Cos(angle) * 0.5f + 0.5f, radiusScale * Math::Sin(angle) * 0.5f + 0.5f);
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
    if (_vbCircleRasterize->Init(GPUBufferDescription::Vertex(sizeof(Float2), vertices, vbData))
        || _ibCircleRasterize->Init(GPUBufferDescription::Index(sizeof(uint16), triangles * 3, ibData)))
    {
        LOG(Fatal, "Failed to setup volumetric fog buffers.");
    }
}
