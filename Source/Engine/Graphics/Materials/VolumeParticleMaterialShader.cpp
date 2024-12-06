// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "VolumeParticleMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Core/Math/Matrix3x4.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/VolumetricFogPass.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Particles/Graph/CPU/ParticleEmitterGraph.CPU.h"

PACK_STRUCT(struct VolumeParticleMaterialShaderData {
    Matrix InverseViewProjectionMatrix;
    Matrix3x4 WorldMatrix;
    Matrix3x4 WorldMatrixInverseTransposed;
    Float3 GridSize;
    float PerInstanceRandom;
    float Dummy0;
    float VolumetricFogMaxDistance;
    int32 ParticleStride;
    int32 ParticleIndex;
    });

DrawPass VolumeParticleMaterialShader::GetDrawModes() const
{
    return DrawPass::None;
}

void VolumeParticleMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    const RenderView& view = params.RenderContext.View;
    auto& drawCall = *params.DrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(VolumeParticleMaterialShaderData));
    auto materialData = reinterpret_cast<VolumeParticleMaterialShaderData*>(cb.Get());
    cb = Span<byte>(cb.Get() + sizeof(VolumeParticleMaterialShaderData), cb.Length() - sizeof(VolumeParticleMaterialShaderData));
    int32 srv = 1;
    auto customData = (VolumetricFogPass::CustomData*)params.CustomData;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = true;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup particles data
    context->BindSR(0, drawCall.Particle.Particles->GPU.Buffer->View());

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
        Matrix::Transpose(view.IVP, materialData->InverseViewProjectionMatrix);
        materialData->WorldMatrix.SetMatrixTranspose(drawCall.World);
        Matrix worldMatrixInverseTransposed;
        Matrix::Invert(drawCall.World, worldMatrixInverseTransposed);
        materialData->WorldMatrixInverseTransposed.SetMatrix(worldMatrixInverseTransposed);
        materialData->GridSize = customData->GridSize;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->VolumetricFogMaxDistance = customData->VolumetricFogMaxDistance;
        materialData->ParticleStride = drawCall.Particle.Particles->Stride;
        materialData->ParticleIndex = customData->ParticleIndex;
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Bind pipeline
    if (_psVolumetricFog == nullptr)
    {
        auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.VS = customData->Shader->GetVS("VS_WriteToSlice");
        psDesc.GS = customData->Shader->GetGS("GS_WriteToSlice");
        psDesc.PS = _shader->GetPS("PS_VolumetricFog");
        _psVolumetricFog = GPUDevice::Instance->CreatePipelineState();
        _psVolumetricFog->Init(psDesc);
    }
    context->SetState(_psVolumetricFog);
}

void VolumeParticleMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    SAFE_DELETE_GPU_RESOURCE(_psVolumetricFog);
}

bool VolumeParticleMaterialShader::Load()
{
    return false;
}
