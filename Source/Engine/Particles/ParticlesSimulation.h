// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Visject/GraphParameter.h"

class ParticleSystemInstance;
class ParticleBuffer;
class ParticleSystem;
class ParticleEmitter;
class GPUBuffer;

/// <summary>
/// Particle system parameter.
/// </summary>
/// <seealso cref="GraphParameter" />
class FLAXENGINE_API ParticleSystemParameter : public GraphParameter
{
public:
    ParticleSystemParameter()
        : GraphParameter(SpawnParams(Guid::New(), TypeInitializer))
    {
    }
};

/// <summary>
/// The particle emitter simulation graph instance data storage. Required to update the particles.
/// </summary>
class FLAXENGINE_API ParticleEmitterInstance
{
public:
    struct SpawnerData
    {
        /// <summary>
        /// The particles spawning counter fractional parts (used to maintain stable spawn rate over time).
        /// </summary>
        float SpawnCounter;

        /// <summary>
        /// The custom data for spawn modules (time of the next spawning).
        /// </summary>
        float NextSpawnTime;
    };

public:
    /// <summary>
    /// The instance data version number. Used to sync the Particle Emitter Graph data with the instance state. Handles Particle Emitter reloads to enure data is valid.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The total simulation time.
    /// </summary>
    float Time;

    /// <summary>
    /// The graph parameters collection (instanced, override the default values).
    /// </summary>
    Array<Variant> Parameters;

    /// <summary>
    /// The particles spawning modules data (one instance per module).
    /// </summary>
    Array<SpawnerData> SpawnModulesData;

    /// <summary>
    /// Custom per-node data (eg. position on spiral module for arc progress tracking)
    /// </summary>
    Array<byte> CustomData;

    /// <summary>
    /// The external amount of the particles to spawn.
    /// </summary>
    int32 CustomSpawnCount = 0;

    struct
    {
        /// <summary>
        /// The accumulated delta time for the GPU simulation update.
        /// </summary>
        float DeltaTime;

        /// <summary>
        /// The accumulated amount of the particles to spawn.
        /// </summary>
        int32 SpawnCount;
    } GPU;

    /// <summary>
    /// The buffer for the particles simulation.
    /// </summary>
    ParticleBuffer* Buffer = nullptr;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleEmitterInstance"/> class.
    /// </summary>
    ParticleEmitterInstance();

    /// <summary>
    /// Finalizes an instance of the <see cref="ParticleEmitterInstance"/> class.
    /// </summary>
    ~ParticleEmitterInstance();

    /// <summary>
    /// Clears this container state data.
    /// </summary>
    void ClearState();

    /// <summary>
    /// Synchronizes the instance data with the specified emitter from the given system.
    /// </summary>
    /// <param name="systemInstance">The system instance data.</param>
    /// <param name="system">The system.</param>
    /// <param name="emitterIndex">The emitter index (in the particle system).</param>
    void Sync(ParticleSystemInstance& systemInstance, ParticleSystem* system, int32 emitterIndex);
};

/// <summary>
/// The particle system simulation graph instance data storage. Required to update the particles.
/// </summary>
class FLAXENGINE_API ParticleSystemInstance
{
public:
    /// <summary>
    /// The instance data version number. Used to sync the Particle System data with the instance state. Handles Particle System reloads to enure data is valid.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The parameters version number. Incremented every time the instance data gets synchronized with system or emitter when it ahs been modified.
    /// </summary>
    uint32 ParametersVersion = 0;

    /// <summary>
    /// The total system playback time.
    /// </summary>
    float Time;

    /// <summary>
    /// The last game time when particle system was updated. Value -1 indicates no previous updates.
    /// </summary>
    float LastUpdateTime = -1;

    /// <summary>
    /// The particle system emitters data (one per emitter instance).
    /// </summary>
    Array<ParticleEmitterInstance> Emitters;

    /// <summary>
    /// The GPU staging readback buffer used to copy the GPU particles count from the GPU buffers and read them on a CPU.
    /// </summary>
    mutable GPUBuffer* GPUParticlesCountReadback = nullptr;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="ParticleSystemInstance"/> class.
    /// </summary>
    ~ParticleSystemInstance();

    /// <summary>
    /// Gets the particles count (total). GPU particles count is read with one frame delay (due to GPU execution).
    /// </summary>
    /// <returns>The particles amount.</returns>
    int32 GetParticlesCount() const;

    /// <summary>
    /// Clears this container state data.
    /// </summary>
    void ClearState();

    /// <summary>
    /// Synchronizes the instance data with the specified system.
    /// </summary>
    /// <param name="system">The system.</param>
    void Sync(ParticleSystem* system);

    /// <summary>
    /// Determines whether the specified emitter is used by this instance.
    /// </summary>
    /// <param name="emitter">The emitter.</param>
    /// <returns><c>true</c> if the specified emitter is used; otherwise, <c>false</c>.</returns>
    bool ContainsEmitter(ParticleEmitter* emitter) const;
};
