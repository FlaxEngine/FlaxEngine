// Copyright (c) Wojciech Figat. All rights reserved.

#include "ParticlesSimulation.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "Particles.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Core/Types/CommonValue.h"

ParticleEmitterInstance::ParticleEmitterInstance()
{
}

ParticleEmitterInstance::~ParticleEmitterInstance()
{
    if (Buffer)
    {
        Particles::RecycleParticleBuffer(Buffer);
    }
}

void ParticleEmitterInstance::ClearState()
{
    Version = 0;
    Time = 0;
    SpawnModulesData.Clear();
    CustomData.Clear();
    CustomSpawnCount = 0;
#if COMPILE_WITH_GPU_PARTICLES
    GPU.DeltaTime = 0.0f;
    GPU.SpawnCount = 0;
#endif
    if (Buffer)
    {
        Particles::RecycleParticleBuffer(Buffer);
        Buffer = nullptr;
    }
}

void ParticleEmitterInstance::Sync(ParticleSystemInstance& systemInstance, ParticleSystem* system, int32 emitterIndex)
{
    auto emitter = system->Emitters[emitterIndex].Get();

    // Sync instance version
    if (Version != emitter->Graph.Version)
    {
        ClearState();
        Version = emitter->Graph.Version;
        systemInstance.ParametersVersion++;

        // Synchronize parameters
        ParticleSystem::EmitterParameterOverrideKey key;
        key.First = emitterIndex;
        Parameters.Resize(emitter->Graph.Parameters.Count(), false);
        for (int32 i = 0; i < emitter->Graph.Parameters.Count(); i++)
        {
            auto& e = emitter->Graph.Parameters.At(i);
            key.Second = e.Identifier;
            if (!system->EmittersParametersOverrides.TryGet(key, Parameters[i]))
                Parameters[i] = e.Value;
        }

        if (SpawnModulesData.Count() != emitter->Graph.SpawnModules.Count())
        {
            SpawnModulesData.Resize(emitter->Graph.SpawnModules.Count(), false);
            SpawnerData data;
            data.SpawnCounter = 0;
            data.NextSpawnTime = 0;
            SpawnModulesData.SetAll(data);
        }
        if (CustomData.Count() != emitter->Graph.CustomDataSize)
        {
            CustomData.Resize(emitter->Graph.CustomDataSize, false);
            Platform::MemoryClear(CustomData.Get(), CustomData.Count());
        }
    }

    // Sync buffer version
    if (Buffer && Buffer->Version != Version)
    {
        Particles::RecycleParticleBuffer(Buffer);
        Buffer = nullptr;
    }
}

ParticleSystemInstance::~ParticleSystemInstance()
{
    SAFE_DELETE_GPU_RESOURCE(GPUParticlesCountReadback);
}

int32 ParticleSystemInstance::GetParticlesCount() const
{
    int32 result = 0;

    // CPU particles
    for (const auto& emitter : Emitters)
    {
        if (emitter.Buffer && emitter.Buffer->Mode == ParticlesSimulationMode::CPU)
        {
            result += emitter.Buffer->CPU.Count;
        }
    }

    // GPU particles
    if (GPUParticlesCountReadback && GPUParticlesCountReadback->IsAllocated())
    {
        auto data = static_cast<uint32*>(GPUParticlesCountReadback->Map(GPUResourceMapMode::Read));
        if (data)
        {
            for (const auto& emitter : Emitters)
            {
                if (emitter.Buffer && emitter.Buffer->Mode == ParticlesSimulationMode::GPU && emitter.Buffer->GPU.HasValidCount)
                {
                    result += *data;
                }
                ++data;
            }
            GPUParticlesCountReadback->Unmap();
        }
    }
    else if (Emitters.HasItems())
    {
        // Initialize readback buffer (next GPU particles simulation update will copy the particle counters)
        if (!GPUParticlesCountReadback)
            GPUParticlesCountReadback = GPUDevice::Instance->CreateBuffer(TEXT("GPUParticlesCountReadback"));
        const auto desc = GPUBufferDescription::Buffer(Emitters.Count() * sizeof(uint32), GPUBufferFlags::None, PixelFormat::Unknown, nullptr, sizeof(uint32), GPUResourceUsage::StagingReadback);
        if (GPUParticlesCountReadback->Init(desc))
        {
            LOG(Error, "Failed to create GPU particles count readback buffer.");
        }
    }

    return result;
}

void ParticleSystemInstance::ClearState()
{
    Version = 0;
    Time = 0;
    LastUpdateTime = -1;
    Emitters.Resize(0);
    if (GPUParticlesCountReadback)
        GPUParticlesCountReadback->ReleaseGPU();
}

void ParticleSystemInstance::Sync(ParticleSystem* system)
{
    // Prepare instance data
    if (Version != system->Version)
    {
        ClearState();
        Version = system->Version;
        ParametersVersion++;
        Emitters.Resize(system->Emitters.Count(), false);
        if (GPUParticlesCountReadback)
            GPUParticlesCountReadback->ReleaseGPU();
    }
    ASSERT(Emitters.Count() == system->Emitters.Count());
}

bool ParticleSystemInstance::ContainsEmitter(ParticleEmitter* emitter) const
{
    for (int32 i = 0; i < Emitters.Count(); i++)
    {
        if (Emitters[i].Buffer && Emitters[i].Buffer->Emitter == emitter)
            return true;
    }
    return false;
}
