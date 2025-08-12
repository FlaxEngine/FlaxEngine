// Copyright (c) Wojciech Figat. All rights reserved.

#include "ParticlesData.h"
#include "ParticleEmitter.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/DynamicBuffer.h"

int32 ParticleAttribute::GetSize() const
{
    switch (ValueType)
    {
    case ValueTypes::Float2:
        return 8;
    case ValueTypes::Float3:
        return 12;
    case ValueTypes::Float4:
        return 16;
    case ValueTypes::Float:
    case ValueTypes::Int:
    case ValueTypes::Uint:
        return 4;
    default:
        return 0;
    }
}

void ParticleLayout::Clear()
{
    Size = 0;
    Attributes.Clear();
}

void ParticleLayout::UpdateLayout()
{
    Size = 0;
    for (int32 i = 0; i < Attributes.Count(); i++)
    {
        Attributes[i].Offset = Size;
        Size += Attributes[i].GetSize();
    }
}

int32 ParticleLayout::FindAttribute(const StringView& name) const
{
    for (int32 i = 0; i < Attributes.Count(); i++)
    {
        if (name == Attributes[i].Name)
            return i;
    }
    return -1;
}

int32 ParticleLayout::FindAttribute(const StringView& name, ParticleAttribute::ValueTypes valueType) const
{
    for (int32 i = 0; i < Attributes.Count(); i++)
    {
        if (Attributes[i].ValueType == valueType && name == Attributes[i].Name)
            return i;
    }
    return -1;
}

int32 ParticleLayout::FindAttributeOffset(const StringView& name, int32 fallbackValue) const
{
    for (int32 i = 0; i < Attributes.Count(); i++)
    {
        if (name == Attributes[i].Name)
            return Attributes[i].Offset;
    }
    return fallbackValue;
}

int32 ParticleLayout::FindAttributeOffset(const StringView& name, ParticleAttribute::ValueTypes valueType, int32 fallbackValue) const
{
    for (int32 i = 0; i < Attributes.Count(); i++)
    {
        if (Attributes[i].ValueType == valueType && name == Attributes[i].Name)
            return Attributes[i].Offset;
    }
    return fallbackValue;
}

int32 ParticleLayout::AddAttribute(const StringView& name, ParticleAttribute::ValueTypes valueType)
{
    auto& a = Attributes.AddOne();
    a.Name = String(*name, name.Length());
    a.ValueType = valueType;
    return Attributes.Count() - 1;
}

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
    SAFE_DELETE(GPU.RibbonVertexBufferDynamic);
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
