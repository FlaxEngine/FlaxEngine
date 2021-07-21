// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ParticlesData.h"
#include "ParticleEmitter.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/DynamicBuffer.h"

ParticleBuffer::ParticleBuffer()
{
}

ParticleBuffer::~ParticleBuffer()
{
    SAFE_DELETE_GPU_RESOURCE(GPU.Buffer);
    SAFE_DELETE_GPU_RESOURCE(GPU.BufferSecondary);
    SAFE_DELETE_GPU_RESOURCE(GPU.IndirectDrawArgsBuffer);
    SAFE_DELETE_GPU_RESOURCE(GPU.SortingKeysBuffer);
    SAFE_DELETE_GPU_RESOURCE(GPU.SortedIndices);
    SAFE_DELETE(GPU.RibbonIndexBufferDynamic);
    for (auto& e : GPU.RibbonSegmentDistances)
        SAFE_DELETE_GPU_RESOURCE(e);
}

bool ParticleBuffer::Init(ParticleEmitter* emitter)
{
    ASSERT(emitter && emitter->IsLoaded());

    Version = emitter->Graph.Version;
    Capacity = emitter->Capacity;
    Emitter = emitter;
    Layout = &emitter->Graph.Layout;
    Stride = Layout->Size;
    Mode = emitter->SimulationMode;

    const int32 size = Capacity * Stride;
    switch (Mode)
    {
    case ParticlesSimulationMode::CPU:
    {
        CPU.Count = 0;
        CPU.Buffer.Resize(size);
        CPU.RibbonOrder.Resize(0);
        GPU.Buffer = GPUDevice::Instance->CreateBuffer(TEXT("ParticleBuffer"));
        if (GPU.Buffer->Init(GPUBufferDescription::Raw(size, GPUBufferFlags::ShaderResource, GPUResourceUsage::Dynamic)))
            return true;
        break;
    }
#if COMPILE_WITH_GPU_PARTICLES
    case ParticlesSimulationMode::GPU:
    {
        if (!emitter->GPU.IsInitialized())
        {
            LOG(Warning, "GPU particles context is not initialized. Cannot create particles buffer.");
            return true;
        }

        // Particle data buffer: attributes + counter + custom data
        GPU.Buffer = GPUDevice::Instance->CreateBuffer(TEXT("ParticleBuffer A"));
        if (GPU.Buffer->Init(GPUBufferDescription::Raw(size + sizeof(uint32) + emitter->GPU.CustomDataSize, GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess)))
            return true;
        GPU.BufferSecondary = GPUDevice::Instance->CreateBuffer(TEXT("ParticleBuffer B"));
        if (GPU.BufferSecondary->Init(GPU.Buffer->GetDescription()))
            return true;
        GPU.IndirectDrawArgsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("ParticleIndirectDrawArgsBuffer"));
        GPU.PendingClear = true;
        GPU.HasValidCount = false;
        GPU.ParticleCounterOffset = size;
        GPU.ParticlesCountMax = 0;
        break;
    }
#endif
    default:
    CRASH;
    }

    return false;
}

bool ParticleBuffer::AllocateSortBuffer()
{
    ASSERT(Emitter && GPU.SortedIndices == nullptr && GPU.SortingKeysBuffer == nullptr);
    if (Emitter->Graph.SortModules.IsEmpty())
        return false;

    switch (Mode)
    {
    case ParticlesSimulationMode::CPU:
    {
        const int32 sortedIndicesSize = Capacity * sizeof(uint32) * Emitter->Graph.SortModules.Count();
        GPU.SortedIndices = GPUDevice::Instance->CreateBuffer(TEXT("SortedIndices"));
        if (GPU.SortedIndices->Init(GPUBufferDescription::Buffer(sortedIndicesSize, GPUBufferFlags::ShaderResource, PixelFormat::R32_UInt, nullptr, sizeof(uint32), GPUResourceUsage::Dynamic)))
            return true;
        break;
    }
#if COMPILE_WITH_GPU_PARTICLES
    case ParticlesSimulationMode::GPU:
    {
        const int32 sortedIndicesSize = Capacity * sizeof(uint32) * Emitter->Graph.SortModules.Count();
        GPU.SortingKeysBuffer = GPUDevice::Instance->CreateBuffer(TEXT("ParticleSortingKeysBuffer"));
        if (GPU.SortingKeysBuffer->Init(GPUBufferDescription::Structured(Capacity, sizeof(float) + sizeof(uint32), true)))
            return true;
        GPU.SortedIndices = GPUDevice::Instance->CreateBuffer(TEXT("SortedIndices"));
        if (GPU.SortedIndices->Init(GPUBufferDescription::Buffer(sortedIndicesSize, GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess, PixelFormat::R32_UInt, nullptr, sizeof(uint32))))
            return true;
        break;
    }
#endif
    default:
    CRASH;
        return true;
    }

    return false;
}

void ParticleBuffer::Clear()
{
    switch (Mode)
    {
    case ParticlesSimulationMode::CPU:
    {
        CPU.Count = 0;
        CPU.RibbonOrder.Clear();
        break;
    }
#if COMPILE_WITH_GPU_PARTICLES
    case ParticlesSimulationMode::GPU:
    {
        GPU.PendingClear = true;
        GPU.HasValidCount = false;
        break;
    }
#endif
    default:
    CRASH;
    }
}
