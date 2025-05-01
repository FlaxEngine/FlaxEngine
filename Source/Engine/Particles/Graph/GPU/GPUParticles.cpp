// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_GPU_PARTICLES

#include "GPUParticles.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Particles/ParticleEffect.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"

GPU_CB_STRUCT(GPUParticlesData {
    Matrix ViewProjectionMatrix;
    Matrix InvViewProjectionMatrix;
    Matrix InvViewMatrix;
    Matrix ViewMatrix;
    Matrix WorldMatrix;
    Matrix InvWorldMatrix;
    Float3 ViewPos;
    float ViewFar;
    Float3 ViewDir;
    float Time;
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float3 EffectPosition;
    float DeltaTime;
    Quaternion EffectRotation;
    Float3 EffectScale;
    uint32 ParticleCounterOffset;
    Float3 Dummy0;
    uint32 SpawnCount;
    });

bool GPUParticles::Init(ParticleEmitter* owner, MemoryReadStream& shaderCacheStream, ReadStream* materialParamsStream, int32 customDataSize)
{
    ASSERT(!_shader);

    // Load shader
    ASSERT(GPUDevice::Instance);
#if GPU_ENABLE_RESOURCE_NAMING
    const StringView name(owner->GetPath());
#else
    const StringView name;
#endif
    _shader = GPUDevice::Instance->CreateShader(name);
    if (_shader->Create(shaderCacheStream))
    {
        LOG(Warning, "Failed to load shader.");
        return true;
    }

    // Setup pipeline
    _mainCS = _shader->GetCS("CS_Main");
    if (!_mainCS)
    {
        LOG(Warning, "Missing CS_Main shader.");
        return true;
    }
    const auto cb0 = _shader->GetCB(0);
    if (!cb0)
    {
        LOG(Warning, "Missing valid GPU particles constant buffer.");
        return true;
    }
    const int32 cbSize = cb0->GetSize();
    if (cbSize < sizeof(GPUParticlesData))
    {
        LOG(Warning, "Invalid size GPU particles constant buffer. required {0} bytes but got {1}", sizeof(GPUParticlesData), cbSize);
        return true;
    }
    _cbData.Resize(cbSize);
    Platform::MemoryClear(_cbData.Get(), cbSize);

    // Load material parameters
    if (_params.Load(materialParamsStream))
    {
        LOG(Warning, "Cannot load material parameters.");
        return true;
    }
    if (_params.Count() < owner->Graph.Parameters.Count())
    {
        LOG(Warning, "Invalid amount of GPU parameters load material parameters. Shader has {0} params, graph has {1}.", _params.Count(), owner->Graph.Parameters.Count());
        return true;
    }

    // Setup custom data size stored by the particle emitter graph generator for the GPU
    ASSERT(customDataSize >= 0 && customDataSize <= 1024);
    CustomDataSize = customDataSize;

    return false;
}

void GPUParticles::Dispose()
{
    // Cleanup
    _mainCS = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_shader);
    _cbData.Resize(0);
    _params.Dispose();
}

void GPUParticles::Update(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt, bool canSpawn)
{
    if (canSpawn)
    {
        // CPU logic controls the particles spawn rate
        data.GPU.SpawnCount += emitter->GraphExecutorCPU.UpdateSpawn(emitter, effect, data, dt);
    }

    // Accumulate delta time for GPU evaluation
    data.GPU.DeltaTime += dt;
}

void GPUParticles::CopyParticlesCount(GPUContext* context, ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, GPUBuffer* dstBuffer, uint32 dstOffset)
{
    if (data.Buffer->GPU.PendingClear || !data.Buffer->GPU.Buffer || !data.Buffer->GPU.HasValidCount)
    {
        uint32 counterDefaultValue = 0;
        context->UpdateBuffer(dstBuffer, &counterDefaultValue, sizeof(counterDefaultValue), dstOffset);
    }
    else
    {
        const uint32 counterOffset = data.Buffer->GPU.ParticleCounterOffset;
        context->CopyBuffer(dstBuffer, data.Buffer->GPU.Buffer, sizeof(uint32), dstOffset, counterOffset);
    }
}

void GPUParticles::Execute(GPUContext* context, ParticleEmitter* emitter, ParticleEffect* effect, int32 emitterIndex, ParticleEmitterInstance& data)
{
    ASSERT(emitter->Graph.Version == data.Version);
    ASSERT(emitter->Graph.Version == data.Buffer->Version);
    uint32 counterDefaultValue = 0;
    const uint32 counterOffset = data.Buffer->GPU.ParticleCounterOffset;
    const bool hasCB = _cbData.HasItems();

    // Clear buffers if need to
    if (data.Buffer->GPU.PendingClear)
    {
        data.Buffer->GPU.PendingClear = false;
        data.Buffer->GPU.ParticlesCountMax = 0;

        // Clear counter in the particles buffer
        context->UpdateBuffer(data.Buffer->GPU.Buffer, &counterDefaultValue, sizeof(counterDefaultValue), counterOffset);

        // Clear custom data
        for (int32 i = 0; i < CustomDataSize; i += 4)
        {
            const uint32 offset = counterOffset + 4 + i * 4;
            context->UpdateBuffer(data.Buffer->GPU.Buffer, &counterDefaultValue, 4, offset);
        }
    }

    // Skip if can
    SceneRenderTask* viewTask = effect->GetRenderTask();
    const int32 threads = data.Buffer->GPU.ParticlesCountMax + data.GPU.SpawnCount;
    if (data.GPU.DeltaTime <= 0.0f || threads == 0 || !_mainCS)
        return;

    // Clear destination buffer counter
    context->UpdateBuffer(data.Buffer->GPU.BufferSecondary, &counterDefaultValue, sizeof(counterDefaultValue), counterOffset);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = hasCB ? ToSpan(_cbData).Slice(sizeof(GPUParticlesData)) : Span<byte>(nullptr, 0);
    bindMeta.Input = nullptr;
    if (viewTask)
    {
        bindMeta.Buffers = viewTask->Buffers;
        bindMeta.CanSampleDepth = bindMeta.CanSampleGBuffer = bindMeta.Buffers && bindMeta.Buffers->GetWidth() != 0;
    }
    else
    {
        bindMeta.Buffers = nullptr;
        bindMeta.CanSampleDepth = bindMeta.CanSampleGBuffer = false;
    }
    ASSERT(data.Parameters.Count() <= _params.Count());
    for (int32 i = 0; i < data.Parameters.Count(); i++)
    {
        // Copy instance parameters values
        _params[i].SetValue(data.Parameters.Get()[i]);
    }
    MaterialParamsLink link;
    link.This = &_params;
    link.Up = link.Down = nullptr;
    MaterialParams::Bind(&link, bindMeta);

    // Setup constant buffer
    if (hasCB)
    {
        auto cbData = (GPUParticlesData*)_cbData.Get();
        if (viewTask)
        {
            auto& view = viewTask->View;

            Matrix::Transpose(view.PrevViewProjection, cbData->ViewProjectionMatrix);

            Matrix tmp;
            Matrix::Invert(view.PrevViewProjection, tmp);
            Matrix::Transpose(tmp, cbData->InvViewProjectionMatrix);

            Matrix::Invert(view.PrevView, tmp);
            Matrix::Transpose(tmp, cbData->InvViewMatrix);

            Matrix::Transpose(view.PrevView, cbData->ViewMatrix);

            cbData->ViewPos = view.Position;
            cbData->ViewFar = view.Far;
            cbData->ViewDir = view.Direction;
            cbData->ViewInfo = view.ViewInfo;
            cbData->ScreenSize = view.ScreenSize;
        }
        else
        {
            Matrix::Transpose(Matrix::Identity, cbData->ViewProjectionMatrix);
            Matrix::Transpose(Matrix::Identity, cbData->InvViewProjectionMatrix);
            Matrix::Transpose(Matrix::Identity, cbData->InvViewMatrix);
            Matrix::Transpose(Matrix::Identity, cbData->ViewMatrix);
            cbData->ViewPos = Float3::Zero;
            cbData->ViewFar = 0.0f;
            cbData->ViewDir = Float3::Forward;
            cbData->ViewInfo = Float4::Zero;
            cbData->ScreenSize = Float4::Zero;
        }
        cbData->Time = data.Time;
        if (emitter->SimulationSpace == ParticlesSimulationSpace::World)
        {
            Matrix::Transpose(Matrix::Identity, cbData->WorldMatrix);
            Matrix::Transpose(Matrix::Identity, cbData->InvWorldMatrix);
        }
        else
        {
            Matrix worldMatrix;
            effect->GetLocalToWorldMatrix(worldMatrix);
            if (viewTask)
                viewTask->View.GetWorldMatrix(worldMatrix);
            Matrix::Transpose(worldMatrix, cbData->WorldMatrix);
            worldMatrix.Invert();
            Matrix::Transpose(worldMatrix, cbData->InvWorldMatrix);
        }
        cbData->Time = data.Time;
        cbData->EffectPosition = effect->GetPosition();
        cbData->DeltaTime = data.GPU.DeltaTime;
        cbData->EffectRotation = effect->GetOrientation();
        cbData->EffectScale = effect->GetScale();
        cbData->ParticleCounterOffset = counterOffset;
        cbData->SpawnCount = data.GPU.SpawnCount;

        // Bind constant buffer
        const auto cb0 = _shader->GetCB(0);
        context->UpdateCB(cb0, cbData);
        context->BindCB(0, cb0);
    }

    // Bind buffers
    context->BindSR(0, data.Buffer->GPU.Buffer->View());
    context->BindUA(0, data.Buffer->GPU.BufferSecondary->View());

    // Invoke Compute shader
    const int32 threadGroupSize = 1024;
    context->Dispatch(_mainCS, Math::Min(Math::DivideAndRoundUp(threads, threadGroupSize), GPU_MAX_CS_DISPATCH_THREAD_GROUPS), 1, 1);

    // Copy custom data
    for (int32 i = 0; i < CustomDataSize; i += 4)
    {
        const uint32 offset = counterOffset + 4 + i * 4;
        context->CopyBuffer(data.Buffer->GPU.Buffer, data.Buffer->GPU.BufferSecondary, 4, offset, offset);
    }

    // Update state
    data.Buffer->GPU.ParticlesCountMax = Math::Min(data.Buffer->GPU.ParticlesCountMax + data.GPU.SpawnCount, data.Buffer->Capacity);
    data.Buffer->GPU.HasValidCount = true;
    data.GPU.DeltaTime = 0.0f;
    data.GPU.SpawnCount = 0;

    // Swap particle buffers
    Swap(data.Buffer->GPU.Buffer, data.Buffer->GPU.BufferSecondary);

    // Copy particles count if need to
    if (effect->Instance.GPUParticlesCountReadback && effect->Instance.GPUParticlesCountReadback->IsAllocated())
    {
        context->CopyBuffer(effect->Instance.GPUParticlesCountReadback, data.Buffer->GPU.Buffer, sizeof(uint32), emitterIndex * sizeof(uint32), counterOffset);
    }
}

#endif
