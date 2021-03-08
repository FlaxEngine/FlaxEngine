// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "VolumeParticleMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
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
    Matrix ViewProjectionMatrix;
    Matrix InverseViewProjectionMatrix;
    Matrix ViewMatrix;
    Matrix WorldMatrix;
    Matrix WorldMatrixInverseTransposed;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    Vector3 GridSize;
    float PerInstanceRandom;
    float Dummy0;
    float VolumetricFogMaxDistance;
    int ParticleStride;
    int ParticleIndex;
    });

DrawPass VolumeParticleMaterialShader::GetDrawModes() const
{
    return DrawPass::None;
}

void VolumeParticleMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<VolumeParticleMaterialShaderData*>(cb);
    cb += sizeof(VolumeParticleMaterialShaderData);
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
                *((int32*)(bindMeta.Constants + param.GetBindOffset())) = offset;
            }
        }
    }

    // Setup material constants
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(view.IVP, materialData->InverseViewProjectionMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Invert(drawCall.World, materialData->WorldMatrixInverseTransposed);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
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
