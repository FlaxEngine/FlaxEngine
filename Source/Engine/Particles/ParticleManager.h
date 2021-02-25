// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

struct RenderContext;
struct RenderView;
class ParticleEmitter;
class ParticleSystemInstance;
class ParticleEffect;
class ParticleSystem;
class ParticleBuffer;
class SceneRenderTask;
class Actor;

/// <summary>
/// The particles service used for simulation and emitters data pooling.
/// </summary>
class FLAXENGINE_API ParticleManager
{
public:

    /// <summary>
    /// Updates the effect during next particles simulation tick.
    /// </summary>
    /// <param name="effect">The effect.</param>
    static void UpdateEffect(ParticleEffect* effect);

    /// <summary>
    /// Called when effect gets removed from gameplay. All references to it should be cleared.
    /// </summary>
    /// <param name="effect">The effect.</param>
    static void OnEffectDestroy(ParticleEffect* effect);

public:

    /// <summary>
    /// Draws the particles.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="effect">The owning actor.</param>
    static void DrawParticles(RenderContext& renderContext, ParticleEffect* effect);

public:

    /// <summary>
    /// Enables or disables particle buffer pooling.
    /// </summary>
    static bool EnableParticleBufferPooling;

    /// <summary>
    /// The particle buffer recycle timeout (in seconds).
    /// </summary>
    static float ParticleBufferRecycleTimeout;

    /// <summary>
    /// Acquires the free particle buffer for the emitter instance data.
    /// </summary>
    /// <param name="emitter">The emitter.</param>
    /// <returns>The particle buffer.</returns>
    static ParticleBuffer* AcquireParticleBuffer(ParticleEmitter* emitter);

    /// <summary>
    /// Recycles the used particle buffer.
    /// </summary>
    /// <param name="buffer">The particle buffer.</param>
    static void RecycleParticleBuffer(ParticleBuffer* buffer);

    /// <summary>
    /// Called when emitter gets unloaded. Particle buffers using this emitter has to be cleared.
    /// </summary>
    /// <param name="emitter">The emitter.</param>
    static void OnEmitterUnload(ParticleEmitter* emitter);
};
