// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ParticleMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/ShadowsPass.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Particles/Graph/CPU/ParticleEmitterGraph.CPU.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

#define MAX_LOCAL_LIGHTS 4

PACK_STRUCT(struct ParticleMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    uint32 SortedIndicesOffset;
    float PerInstanceRandom;
    int32 ParticleStride;
    int32 PositionOffset;
    int32 SpriteSizeOffset;
    int32 SpriteFacingModeOffset;
    int32 SpriteFacingVectorOffset;
    int32 VelocityOffset;
    int32 RotationOffset;
    int32 ScaleOffset;
    int32 ModelFacingModeOffset;
    float RibbonUVTilingDistance;
    Vector2 RibbonUVScale;
    Vector2 RibbonUVOffset;
    int32 RibbonWidthOffset;
    int32 RibbonTwistOffset;
    int32 RibbonFacingVectorOffset;
    uint32 RibbonSegmentCount;
    Matrix WorldMatrixInverseTransposed;
    });

PACK_STRUCT(struct ParticleMaterialShaderLightingData {
    LightData DirectionalLight;
    LightShadowData DirectionalLightShadow;
    LightData SkyLight;
    ProbeData EnvironmentProbe;
    ExponentialHeightFogData ExponentialHeightFog;
    Vector3 Dummy1;
    uint32 LocalLightsCount;
    LightData LocalLights[MAX_LOCAL_LIGHTS];
    });

DrawPass ParticleMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

void ParticleMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto cache = params.RenderContext.List;
    auto& drawCall = *params.FirstDrawCall;
    const auto cb0 = _shader->GetCB(0);
    const bool hasCb0 = cb0->GetSize() != 0;
    const auto cb1 = _shader->GetCB(1);
    const bool hasCb1 = cb1->GetSize() != 0;
    const uint32 sortedIndicesOffset = drawCall.Module->SortedIndicesOffset;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Buffer0 = hasCb0 ? _cb0Data.Get() + sizeof(ParticleMaterialShaderData) : nullptr;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = GPUDevice::Instance->Limits.HasReadOnlyDepth;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup particles data and attributes binding info
    {
        context->BindSR(0, drawCall.Particles->GPU.Buffer->View());
        if (drawCall.Particles->GPU.SortedIndices)
            context->BindSR(1, drawCall.Particles->GPU.SortedIndices->View());

        if (hasCb0)
        {
            const auto& p = *params.ParamsLink->This;
            for (int32 i = 0; i < p.Count(); i++)
            {
                const auto& param = p.At(i);
                if (param.GetParameterType() == MaterialParameterType::Integer && param.GetName().StartsWith(TEXT("Particle.")))
                {
                    auto name = StringView(param.GetName().Get() + 9);

                    const int32 offset = drawCall.Particles->Layout->FindAttributeOffset(name);
                    *((int32*)(bindMeta.Buffer0 + param.GetBindOffset())) = offset;
                }
            }
        }
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
    PipelineStateCache* psCache = nullptr;
    switch (drawCall.Module->TypeID)
    {
        // Sprite Rendering
    case 400:
    {
        psCache = (PipelineStateCache*)_cacheSprite.GetPS(view.Pass);
        break;
    }
        // Model Rendering
    case 403:
    {
        psCache = (PipelineStateCache*)_cacheModel.GetPS(view.Pass);
        break;
    }
        // Ribbon Rendering
    case 404:
    {
        psCache = (PipelineStateCache*)_cacheRibbon.GetPS(view.Pass);

        static StringView ParticleRibbonWidth(TEXT("RibbonWidth"));
        static StringView ParticleRibbonTwist(TEXT("RibbonTwist"));
        static StringView ParticleRibbonFacingVector(TEXT("RibbonFacingVector"));

        if (hasCb0)
        {
            const auto materialData = reinterpret_cast<ParticleMaterialShaderData*>(_cb0Data.Get());

            materialData->RibbonWidthOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleRibbonWidth, ParticleAttribute::ValueTypes::Float, -1);
            materialData->RibbonTwistOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleRibbonTwist, ParticleAttribute::ValueTypes::Float, -1);
            materialData->RibbonFacingVectorOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleRibbonFacingVector, ParticleAttribute::ValueTypes::Vector3, -1);

            materialData->RibbonUVTilingDistance = drawCall.Ribbon.UVTilingDistance;
            materialData->RibbonUVScale.X = drawCall.Ribbon.UVScaleX;
            materialData->RibbonUVScale.Y = drawCall.Ribbon.UVScaleY;
            materialData->RibbonUVOffset.X = drawCall.Ribbon.UVOffsetX;
            materialData->RibbonUVOffset.Y = drawCall.Ribbon.UVOffsetY;
            materialData->RibbonSegmentCount = drawCall.Ribbon.SegmentCount;
        }

        if (drawCall.Ribbon.SegmentDistances)
            context->BindSR(1, drawCall.Ribbon.SegmentDistances->View());

        break;
    }
    }
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Setup material constants data
    if (hasCb0)
    {
        const auto materialData = reinterpret_cast<ParticleMaterialShaderData*>(_cb0Data.Get());

        static StringView ParticlePosition(TEXT("Position"));
        static StringView ParticleSpriteSize(TEXT("SpriteSize"));
        static StringView ParticleSpriteFacingMode(TEXT("SpriteFacingMode"));
        static StringView ParticleSpriteFacingVector(TEXT("SpriteFacingVector"));
        static StringView ParticleVelocityOffset(TEXT("Velocity"));
        static StringView ParticleRotationOffset(TEXT("Rotation"));
        static StringView ParticleScaleOffset(TEXT("Scale"));
        static StringView ParticleModelFacingModeOffset(TEXT("ModelFacingMode"));

        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = Time::Draw.UnscaledTime.GetTotalSeconds();
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        materialData->SortedIndicesOffset = drawCall.Particles->GPU.SortedIndices && params.RenderContext.View.Pass != DrawPass::Depth ? sortedIndicesOffset : 0xFFFFFFFF;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->ParticleStride = drawCall.Particles->Stride;
        materialData->PositionOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticlePosition, ParticleAttribute::ValueTypes::Vector3);
        materialData->SpriteSizeOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleSpriteSize, ParticleAttribute::ValueTypes::Vector2);
        materialData->SpriteFacingModeOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingMode, ParticleAttribute::ValueTypes::Int, -1);
        materialData->SpriteFacingVectorOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingVector, ParticleAttribute::ValueTypes::Vector3);
        materialData->VelocityOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleVelocityOffset, ParticleAttribute::ValueTypes::Vector3);
        materialData->RotationOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleRotationOffset, ParticleAttribute::ValueTypes::Vector3, -1);
        materialData->ScaleOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleScaleOffset, ParticleAttribute::ValueTypes::Vector3, -1);
        materialData->ModelFacingModeOffset = drawCall.Particles->Layout->FindAttributeOffset(ParticleModelFacingModeOffset, ParticleAttribute::ValueTypes::Int, -1);
        Matrix::Invert(drawCall.World, materialData->WorldMatrixInverseTransposed);
    }

    // Setup lighting constants data
    if (hasCb1)
    {
        auto& lightingData = *reinterpret_cast<ParticleMaterialShaderLightingData*>(_cb1Data.Get());
        const int32 envProbeShaderRegisterIndex = 2;
        const int32 skyLightShaderRegisterIndex = 3;
        const int32 dirLightShaderRegisterIndex = 4;

        // Set fog input
        if (cache->Fog)
        {
            cache->Fog->GetExponentialHeightFogData(view, lightingData.ExponentialHeightFog);
        }
        else
        {
            lightingData.ExponentialHeightFog.FogMinOpacity = 1.0f;
            lightingData.ExponentialHeightFog.ApplyDirectionalInscattering = 0.0f;
        }

        // Set directional light input
        if (cache->DirectionalLights.HasItems())
        {
            const auto& dirLight = cache->DirectionalLights.First();
            const auto shadowPass = ShadowsPass::Instance();
            const bool useShadow = shadowPass->LastDirLightIndex == 0;
            if (useShadow)
            {
                lightingData.DirectionalLightShadow = shadowPass->LastDirLight;
                context->BindSR(dirLightShaderRegisterIndex, shadowPass->LastDirLightShadowMap);
            }
            else
            {
                context->UnBindSR(dirLightShaderRegisterIndex);
            }
            dirLight.SetupLightData(&lightingData.DirectionalLight, view, useShadow);
        }
        else
        {
            lightingData.DirectionalLight.Color = Vector3::Zero;
            lightingData.DirectionalLight.CastShadows = 0.0f;
            context->UnBindSR(dirLightShaderRegisterIndex);
        }

        // Set sky light
        if (cache->SkyLights.HasItems())
        {
            auto& skyLight = cache->SkyLights.Last();
            skyLight.SetupLightData(&lightingData.SkyLight, view, false);
            const auto texture = skyLight.Image ? skyLight.Image->GetTexture() : nullptr;
            context->BindSR(skyLightShaderRegisterIndex, texture);
        }
        else
        {
            Platform::MemoryClear(&lightingData.SkyLight, sizeof(lightingData.SkyLight));
            context->UnBindSR(skyLightShaderRegisterIndex);
        }

        // Set reflection probe data
        EnvironmentProbe* probe = nullptr;
        // TODO: optimize env probe searching for a transparent material - use spatial cache for renderer to find it
        for (int32 i = 0; i < cache->EnvironmentProbes.Count(); i++)
        {
            const auto p = cache->EnvironmentProbes[i];
            if (p->GetSphere().Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                probe = p;
                break;
            }
        }
        if (probe && probe->GetProbe())
        {
            probe->SetupProbeData(&lightingData.EnvironmentProbe);
            const auto texture = probe->GetProbe()->GetTexture();
            context->BindSR(envProbeShaderRegisterIndex, texture);
        }
        else
        {
            lightingData.EnvironmentProbe.Data1 = Vector4::Zero;
            context->UnBindSR(envProbeShaderRegisterIndex);
        }

        // Set local lights
        lightingData.LocalLightsCount = 0;
        for (int32 i = 0; i < cache->PointLights.Count(); i++)
        {
            const auto& light = cache->PointLights[i];
            if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                light.SetupLightData(&lightingData.LocalLights[lightingData.LocalLightsCount], view, false);
                lightingData.LocalLightsCount++;
                if (lightingData.LocalLightsCount == MAX_LOCAL_LIGHTS)
                    break;
            }
        }
        for (int32 i = 0; i < cache->SpotLights.Count(); i++)
        {
            const auto& light = cache->SpotLights[i];
            if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                light.SetupLightData(&lightingData.LocalLights[lightingData.LocalLightsCount], view, false);
                lightingData.LocalLightsCount++;
                if (lightingData.LocalLightsCount == MAX_LOCAL_LIGHTS)
                    break;
            }
        }
    }

    // Bind constants
    if (hasCb0)
    {
        context->UpdateCB(cb0, _cb0Data.Get());
        context->BindCB(0, cb0);
    }
    if (hasCb1)
    {
        context->UpdateCB(cb1, _cb1Data.Get());
        context->BindCB(1, cb1);
    }

    // Bind pipeline
    context->SetState(state);
}

void ParticleMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cacheSprite.Release();
    _cacheModel.Release();
    _cacheRibbon.Release();
}

bool ParticleMaterialShader::Load()
{
    _drawModes = DrawPass::Depth | DrawPass::Forward;
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthTestEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == 0;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == 0;

    auto vsSprite = _shader->GetVS("VS_Sprite");
    auto vsMesh = _shader->GetVS("VS_Model");
    auto vsRibbon = _shader->GetVS("VS_Ribbon");

    // Check if use transparent distortion pass
    if (_shader->HasShader("PS_Distortion"))
    {
        _drawModes |= DrawPass::Distortion;

        // Accumulate Distortion Pass
        psDesc.PS = _shader->GetPS("PS_Distortion");
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.DepthWriteEnable = false;
        psDesc.VS = vsSprite;
        _cacheSprite.Distortion.Init(psDesc);
        psDesc.VS = vsMesh;
        _cacheModel.Distortion.Init(psDesc);
        psDesc.VS = vsRibbon;
        _cacheRibbon.Distortion.Init(psDesc);
    }

    // Forward Pass
    psDesc.PS = _shader->GetPS("PS_Forward");
    psDesc.DepthWriteEnable = false;
    psDesc.BlendMode = BlendingMode::AlphaBlend;
    switch (_info.BlendMode)
    {
    case MaterialBlendMode::Transparent:
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        break;
    case MaterialBlendMode::Additive:
        psDesc.BlendMode = BlendingMode::Additive;
        break;
    case MaterialBlendMode::Multiply:
        psDesc.BlendMode = BlendingMode::Multiply;
        break;
    }
    psDesc.VS = vsSprite;
    _cacheSprite.Default.Init(psDesc);
    psDesc.VS = vsMesh;
    _cacheModel.Default.Init(psDesc);
    psDesc.VS = vsRibbon;
    _cacheRibbon.Default.Init(psDesc);

    // Depth Pass
    psDesc = GPUPipelineState::Description::Default;
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.PS = _shader->GetPS("PS_Depth");
    psDesc.VS = vsSprite;
    _cacheSprite.Depth.Init(psDesc);
    psDesc.VS = vsMesh;
    _cacheModel.Depth.Init(psDesc);
    psDesc.VS = vsRibbon;
    _cacheRibbon.Depth.Init(psDesc);

    return false;
}
