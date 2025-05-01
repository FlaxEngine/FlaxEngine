// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_GPU_PARTICLES

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Materials/MaterialParams.h"

// Forward declarations
class MemoryReadStream;
class ParticleEmitterInstance;
class ParticleEffect;
class ParticleEmitter;
class GPUContext;
class GPUBuffer;
class GPUShader;
class GPUShaderProgramCS;

/// <summary>
/// The GPU particles execution utility.
/// </summary>
class GPUParticles
{
private:
    GPUShader* _shader = nullptr;
    GPUShaderProgramCS* _mainCS = nullptr;
    Array<byte> _cbData;
    MaterialParams _params;

public:
    /// <summary>
    /// The custom data size (in bytes) required by the nodes to store the additional global state for the simulation in the particles buffer on a GPU.
    /// </summary>
    int32 CustomDataSize;

    /// <summary>
    /// Determines whether this instance is initialized.
    /// </summary>
    /// <returns><c>true</c> if this instance is initialized; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsInitialized() const
    {
        return _shader != nullptr;
    }

    /// <summary>
    /// Initializes the GPU particles simulation runtime.
    /// </summary>
    /// <param name="owner">The owning emitter.</param>
    /// <param name="shaderCacheStream">The stream with the compiled shader data.</param>
    /// <param name="materialParamsStream">The stream with the parameters data.</param>
    /// <param name="customDataSize">The custom data size (in bytes) required by the nodes to store the additional global state for the simulation in the particles buffer on a GPU.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(ParticleEmitter* owner, MemoryReadStream& shaderCacheStream, ReadStream* materialParamsStream, int32 customDataSize);

    /// <summary>
    /// Releases the resources.
    /// </summary>
    void Dispose();

    /// <summary>
    /// Updates the particles simulation (the GPU simulation). The actual simulation is performed during Execute during rendering. This method accumulates the simulation delta time and other properties.
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="dt">The delta time (in seconds).</param>
    /// <param name="canSpawn">True if can spawn new particles, otherwise will just perform an update.</param>
    void Update(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt, bool canSpawn);

    /// <summary>
    /// Copies the GPU particles count from the particles data on a GPU to another GPU buffer (counter value is uint32).
    /// </summary>
    /// <param name="context">The GPU context that supports Compute.</param>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="dstBuffer">The destination buffer to copy the counter (uint32).</param>
    /// <param name="dstOffset">The destination buffer offset from start (in bytes) to copy the counter (uint32).</param>
    void CopyParticlesCount(GPUContext* context, ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, GPUBuffer* dstBuffer, uint32 dstOffset);

    /// <summary>
    /// Performs the GPU particles simulation update using the graphics device.
    /// </summary>
    /// <param name="context">The GPU context that supports Compute.</param>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="emitterIndex">The index of the emitter in the particle system.</param>
    /// <param name="data">The instance data.</param>
    void Execute(GPUContext* context, ParticleEmitter* emitter, ParticleEffect* effect, int32 emitterIndex, ParticleEmitterInstance& data);
};

#endif
