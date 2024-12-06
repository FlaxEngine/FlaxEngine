// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ParticleMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Core/Math/Matrix3x4.h"
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
    Matrix3x4 WorldMatrix;
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
    Float2 RibbonUVScale;
    Float2 RibbonUVOffset;
    int32 RibbonWidthOffset;
    int32 RibbonTwistOffset;
    int32 RibbonFacingVectorOffset;
    uint32 RibbonSegmentCount;
    Matrix3x4 WorldMatrixInverseTransposed;
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
    auto& drawCall = *params.DrawCall;
    const uint32 sortedIndicesOffset = drawCall.Particle.Module->SortedIndicesOffset;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(ParticleMaterialShaderData));
    auto materialData = reinterpret_cast<ParticleMaterialShaderData*>(cb.Get());
    cb = Span<byte>(cb.Get() + sizeof(ParticleMaterialShaderData), cb.Length() - sizeof(ParticleMaterialShaderData));
    int32 srv = 2;

    // Setup features
    if (EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::GlobalIllumination))
        GlobalIlluminationFeature::Bind(params, cb, srv);
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
                ASSERT_LOW_LAYER(bindMeta.Constants.Get() && bindMeta.Constants.Length() >= (int32)(param.GetBindOffset() + sizeof(int32)));
                *((int32*)(bindMeta.Constants.Get() + param.GetBindOffset())) = offset;
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

        materialData->WorldMatrix.SetMatrixTranspose(drawCall.World);
        materialData->SortedIndicesOffset = drawCall.Particle.Particles->GPU.SortedIndices && params.RenderContext.View.Pass != DrawPass::Depth ? sortedIndicesOffset : 0xFFFFFFFF;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->ParticleStride = drawCall.Particle.Particles->Stride;
        materialData->PositionOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticlePosition, ParticleAttribute::ValueTypes::Float3);
        materialData->SpriteSizeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteSize, ParticleAttribute::ValueTypes::Float2);
        materialData->SpriteFacingModeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingMode, ParticleAttribute::ValueTypes::Int, -1);
        materialData->SpriteFacingVectorOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleSpriteFacingVector, ParticleAttribute::ValueTypes::Float3);
        materialData->VelocityOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleVelocityOffset, ParticleAttribute::ValueTypes::Float3);
        materialData->RotationOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRotationOffset, ParticleAttribute::ValueTypes::Float3, -1);
        materialData->ScaleOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleScaleOffset, ParticleAttribute::ValueTypes::Float3, -1);
        materialData->ModelFacingModeOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleModelFacingModeOffset, ParticleAttribute::ValueTypes::Int, -1);
        Matrix worldMatrixInverseTransposed;
        Matrix::Invert(drawCall.World, worldMatrixInverseTransposed);
        materialData->WorldMatrixInverseTransposed.SetMatrix(worldMatrixInverseTransposed);
    }

    // Select pipeline state based on current pass and render mode
    bool wireframe = EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::Wireframe) || view.Mode == ViewMode::Wireframe;
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
        materialData->RibbonFacingVectorOffset = drawCall.Particle.Particles->Layout->FindAttributeOffset(ParticleRibbonFacingVector, ParticleAttribute::ValueTypes::Float3, -1);

        materialData->RibbonUVTilingDistance = drawCall.Particle.Ribbon.UVTilingDistance;
        materialData->RibbonUVScale.X = drawCall.Particle.Ribbon.UVScaleX;
        materialData->RibbonUVScale.Y = drawCall.Particle.Ribbon.UVScaleY;
        materialData->RibbonUVOffset.X = drawCall.Particle.Ribbon.UVOffsetX;
        materialData->RibbonUVOffset.Y = drawCall.Particle.Ribbon.UVOffsetY;
        materialData->RibbonSegmentCount = drawCall.Particle.Ribbon.SegmentCount;

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
    _drawModes = DrawPass::Depth | DrawPass::Forward | DrawPass::QuadOverdraw;
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == MaterialFeaturesFlags::None;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == MaterialFeaturesFlags::None;

    auto vsSprite = _shader->GetVS("VS_Sprite");
    auto vsMesh = _shader->GetVS("VS_Model");
    auto vsRibbon = _shader->GetVS("VS_Ribbon");

#if USE_EDITOR
    if (_shader->HasShader("PS_QuadOverdraw"))
    {
        // Quad Overdraw
        psDesc.PS = _shader->GetPS("PS_QuadOverdraw");
        psDesc.VS = vsSprite;
        _cacheSprite.QuadOverdraw.Init(psDesc);
        psDesc.VS = vsMesh;
        _cacheModel.QuadOverdraw.Init(psDesc);
        psDesc.VS = vsRibbon;
        _cacheRibbon.QuadOverdraw.Init(psDesc);
    }
#endif

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
