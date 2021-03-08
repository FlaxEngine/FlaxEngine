// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ParticleMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Particles/Graph/CPU/ParticleEmitterGraph.CPU.h"

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

DrawPass ParticleMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

void ParticleMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    const uint32 sortedIndicesOffset = drawCall.Particle.Module->SortedIndicesOffset;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<ParticleMaterialShaderData*>(cb);
    cb += sizeof(ParticleMaterialShaderData);
    int32 srv = 2;

    // Setup features
    ForwardShadingFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = GPUDevice::Instance->Limits.HasReadOnlyDepth;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup particles data
    context->BindSR(0, drawCall.Particle.Particles->GPU.Buffer->View());
    context->BindSR(1, drawCall.Particle.Particles->GPU.SortedIndices ? drawCall.Particle.Particles->GPU.SortedIndices->View() : nullptr);

    // Setup particles attributes binding info
    {
        const auto& p = *params.ParamsLink->This;
        for (int32 i = 0; i < p.Count(); i++)
        {
            const auto& param = p.At(i);
            if (param.GetParameterType() == MaterialParameterType::Integer && param.GetName().StartsWith(TEXT("Particle.")))
            {
                const StringView name(param.GetName().Get() + 9, param.GetName().Length() - 9);
                const int32 offset = drawCall.Particle.Particles->Layout->FindAttributeOffset(name);
                *((int32*)(bindMeta.Constants + param.GetBindOffset())) = offset;
            }
        }
    }

    // Setup material constants
    {
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
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        materialData->SortedIndicesOffset = drawCall.Particle.Particles->GPU.SortedIndices && params.RenderContext.View.Pass != DrawPass::Depth ? sortedIndicesOffset : 0xFFFFFFFF;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->ParticleStride = drawCall.Particle.Particles->Stride;
        materialData->PositionOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticlePosition, ParticleAttribute::ValueTypes::Vector3);
        materialData->SpriteSizeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteSize, ParticleAttribute::ValueTypes::Vector2);
        materialData->SpriteFacingModeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingMode, ParticleAttribute::ValueTypes::Int, -1);
        materialData->SpriteFacingVectorOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingVector, ParticleAttribute::ValueTypes::Vector3);
        materialData->VelocityOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleVelocityOffset, ParticleAttribute::ValueTypes::Vector3);
        materialData->RotationOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRotationOffset, ParticleAttribute::ValueTypes::Vector3, -1);
        materialData->ScaleOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleScaleOffset, ParticleAttribute::ValueTypes::Vector3, -1);
        materialData->ModelFacingModeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleModelFacingModeOffset, ParticleAttribute::ValueTypes::Int, -1);
        Matrix::Invert(drawCall.World, materialData->WorldMatrixInverseTransposed);
    }

    // Select pipeline state based on current pass and render mode
    bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
    PipelineStateCache* psCache = nullptr;
    switch (drawCall.Particle.Module->TypeID)
    {
        // Sprite Rendering
    case 400:
    {
        psCache = _cacheSprite.GetPS(view.Pass);
        break;
    }
        // Model Rendering
    case 403:
    {
        psCache = _cacheModel.GetPS(view.Pass);
        break;
    }
        // Ribbon Rendering
    case 404:
    {
        psCache = _cacheRibbon.GetPS(view.Pass);

        static StringView ParticleRibbonWidth(TEXT("RibbonWidth"));
        static StringView ParticleRibbonTwist(TEXT("RibbonTwist"));
        static StringView ParticleRibbonFacingVector(TEXT("RibbonFacingVector"));

        materialData->RibbonWidthOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRibbonWidth, ParticleAttribute::ValueTypes::Float, -1);
        materialData->RibbonTwistOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRibbonTwist, ParticleAttribute::ValueTypes::Float, -1);
        materialData->RibbonFacingVectorOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRibbonFacingVector, ParticleAttribute::ValueTypes::Vector3, -1);

        materialData->RibbonUVTilingDistance = drawCall.Particle.Ribbon.UVTilingDistance;
        materialData->RibbonUVScale.X = drawCall.Particle.Ribbon.UVScaleX;
        materialData->RibbonUVScale.Y = drawCall.Particle.Ribbon.UVScaleY;
        materialData->RibbonUVOffset.X = drawCall.Particle.Ribbon.UVOffsetX;
        materialData->RibbonUVOffset.Y = drawCall.Particle.Ribbon.UVOffsetY;
        materialData->RibbonSegmentCount = drawCall.Particle.Ribbon.SegmentCount;

        if (drawCall.Particle.Ribbon.SegmentDistances)
            context->BindSR(1, drawCall.Particle.Ribbon.SegmentDistances->View());

        break;
    }
    }
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
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
    _cacheVolumetricFog.Release();
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

    // Lazy initialization
    _cacheVolumetricFog.Desc.PS = nullptr;

    return false;
}
