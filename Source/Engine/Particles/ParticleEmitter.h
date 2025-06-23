// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Graphics/Shaders/Cache/ShaderAssetBase.h"
#include "Graph/CPU/ParticleEmitterGraph.CPU.h"
#if COMPILE_WITH_GPU_PARTICLES
#include "Graph/GPU/GPUParticles.h"
#endif

class Actor;
class ParticleEffect;
class ParticleEmitterInstance;

/// <summary>
/// Binary asset that contains a particle emitter definition graph for running particles simulation on CPU and GPU.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API ParticleEmitter : public ShaderAssetTypeBase<BinaryAsset>
{
    DECLARE_BINARY_ASSET_HEADER(ParticleEmitter, ShadersSerializedVersion);

public:
    /// <summary>
    /// The loaded particle graph.
    /// </summary>
    ParticleEmitterGraphCPU Graph;

    /// <summary>
    /// The CPU graph executor runtime.
    /// </summary>
    ParticleEmitterGraphCPUExecutor GraphExecutorCPU;

    /// <summary>
    /// The particle system capacity (the maximum amount of particles to simulate at once).
    /// </summary>
    int32 Capacity;

    /// <summary>
    /// The particles simulation execution mode.
    /// </summary>
    ParticlesSimulationMode SimulationMode;

    /// <summary>
    /// The particles simulation space.
    /// </summary>
    ParticlesSimulationSpace SimulationSpace;

    /// <summary>
    /// True if enable pooling emitter instance data, otherwise immediately dispose. Pooling can improve performance and reduce memory usage.
    /// </summary>
    bool EnablePooling;

    /// <summary>
    /// True if use automatic bounds.
    /// </summary>
    bool UseAutoBounds;

    /// <summary>
    /// True if emitter uses lights rendering, otherwise false.
    /// </summary>
    bool IsUsingLights;

    /// <summary>
    /// The custom bounds to use for the particles. Set to zero to use automatic bounds (valid only for CPU particles).
    /// </summary>
    BoundingBox CustomBounds;

#if COMPILE_WITH_GPU_PARTICLES

    /// <summary>
    /// The GPU particle simulation handler.
    /// </summary>
    GPUParticles GPU;

#endif

public:
    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <param name="createDefaultIfMissing">True if create default surface if missing.</param>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadSurface(bool createDefaultIfMissing);

#if USE_EDITOR
    /// <summary>
    /// Updates surface (saves new one, discard cached data, reloads asset).
    /// </summary>
    /// <param name="data">The surface graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data);
#endif

public:
    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="position">The spawn position.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Vector3& position, float duration = MAX_float, bool autoDestroy = false)
    {
        return Spawn(nullptr, Transform(position), duration, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="position">The spawn position.</param>
    /// <param name="rotation">The spawn rotation.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Vector3& position, const Quaternion& rotation, float duration = MAX_float, bool autoDestroy = false)
    {
        return Spawn(nullptr, Transform(position, rotation), duration, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="transform">The spawn transform.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Transform& transform, float duration = MAX_float, bool autoDestroy = false)
    {
        return Spawn(nullptr, transform, duration, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="position">The spawn position.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, const Vector3& position, float duration = MAX_float, bool autoDestroy = false)
    {
        return Spawn(parent, Transform(position), duration, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="position">The spawn position.</param>
    /// <param name="rotation">The spawn rotation.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, const Vector3& position, const Quaternion& rotation, float duration = MAX_float, bool autoDestroy = false)
    {
        return Spawn(parent, Transform(position, rotation), duration, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="transform">The spawn transform.</param>
    /// <param name="duration">The effect playback duration (in seconds).</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect will be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, const Transform& transform, float duration = MAX_float, bool autoDestroy = false);

private:
    void WaitForAsset(Asset* asset);

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;

    /// <summary>
    /// Checks if the particle emitter has valid shader code present.
    /// </summary>
    API_PROPERTY() bool HasShaderCode() const;
#endif

protected:
    // [ParticleEmitterBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
#if USE_EDITOR
    void OnDependencyModified(BinaryAsset* asset) override;
    void InitCompilationOptions(ShaderCompilationOptions& options) override;
#endif
};
