// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Graphics/RenderTask.h"
#include "ParticleSystem.h"
#include "ParticlesSimulation.h"

/// <summary>
/// Particle system parameter.
/// </summary>
API_CLASS(NoSpawn) class ParticleEffectParameter : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ParticleEffectParameter);
    friend ParticleEffect;
private:
    ParticleEffect* _effect = nullptr;
    int32 _emitterIndex;
    int32 _paramIndex;

    void Init(ParticleEffect* effect, int32 emitterIndex, int32 paramIndex);

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleEffectParameter"/> class.
    /// </summary>
    ParticleEffectParameter()
        : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    {
    }

    /// <summary>
    /// Returns true if parameter object handle is valid.
    /// </summary>
    bool IsValid() const;

    /// <summary>
    /// Gets the index of the emitter (not the emitter track but the emitter).
    /// </summary>
    API_PROPERTY() int32 GetEmitterIndex() const
    {
        return _emitterIndex;
    }

    /// <summary>
    /// Gets the emitter that this parameter belongs to.
    /// </summary>
    API_PROPERTY() ParticleEmitter* GetEmitter() const;

    /// <summary>
    /// Gets the parameter index (in the emitter parameters list).
    /// </summary>
    API_PROPERTY() int32 GetParamIndex() const
    {
        return _paramIndex;
    }

    /// <summary>
    /// Gets the parameter type.
    /// </summary>
    API_PROPERTY() VariantType GetParamType() const;

    /// <summary>
    /// Gets the parameter unique ID.
    /// </summary>
    API_PROPERTY() Guid GetParamIdentifier() const;

    /// <summary>
    /// Gets the emitter track name.
    /// </summary>
    API_PROPERTY() const String& GetTrackName() const;

    /// <summary>
    /// Gets the parameter name.
    /// </summary>
    API_PROPERTY() const String& GetName() const;

    /// <summary>
    /// Gets the parameter flag that indicates whenever it's exposed to public.
    /// </summary>
    API_PROPERTY() bool GetIsPublic() const;

    /// <summary>
    /// Gets the default value of the parameter (set in particle system asset).
    /// </summary>
    /// <returns>The default value.</returns>
    API_PROPERTY() Variant GetDefaultValue() const;

    /// <summary>
    /// Gets the default value of the parameter (set in particle emitter asset).
    /// </summary>
    /// <returns>The default value.</returns>
    API_PROPERTY() const Variant& GetDefaultEmitterValue() const;

    /// <summary>
    /// Gets the value of the parameter.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY() const Variant& GetValue() const;

    /// <summary>
    /// Sets the value of the parameter.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetValue(const Variant& value) const;

    /// <summary>
    /// Gets the particle emitter parameter from the asset (the parameter instanced by this object).
    /// </summary>
    /// <returns>The particle emitter parameter overriden by this object.</returns>
    API_PROPERTY() GraphParameter* GetEmitterParameter() const;
};

/// <summary>
/// The particle system instance that plays the particles simulation in the game.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Particle Effect\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API ParticleEffect : public Actor
{
    DECLARE_SCENE_OBJECT(ParticleEffect);
public:
    /// <summary>
    /// The particles simulation update modes.
    /// </summary>
    API_ENUM() enum class SimulationUpdateMode
    {
        /// <summary>
        /// Use realtime simulation updates. Updates particles during every game logic update.
        /// </summary>
        Realtime = 0,

        /// <summary>
        /// Use fixed timestep delta time to update particles simulation with a custom frequency.
        /// </summary>
        FixedTimestep = 1,
    };

    /// <summary>
    /// The particle parameter override data.
    /// </summary>
    API_STRUCT() struct ParameterOverride
    {
        DECLARE_SCRIPTING_TYPE_NO_SPAWN(ParameterOverride);

        /// <summary>
        /// The name of the track that has overriden parameter.
        /// </summary>
        API_FIELD() String Track;

        /// <summary>
        /// The overriden parameter id.
        /// </summary>
        API_FIELD() Guid Id;

        /// <summary>
        /// The overriden value.
        /// </summary>
        API_FIELD() Variant Value;
    };

private:
    uint64 _lastUpdateFrame;
    Real _lastMinDstSqr;
    int32 _sceneRenderingKey = -1;
    uint32 _parametersVersion = 0; // Version number for _parameters to be in sync with Instance.ParametersVersion
    Array<ParticleEffectParameter> _parameters; // Cached for scripting API
    Array<ParameterOverride> _parametersOverrides; // Cached parameter modifications to be applied to the parameters
    bool _isPlaying = false;
    bool _isStopped = false;

public:
    /// <summary>
    /// The particle system to play.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(null), EditorOrder(0)")
    AssetReference<ParticleSystem> ParticleSystem;

    /// <summary>
    /// The instance data of the particle system.
    /// </summary>
    ParticleSystemInstance Instance;

    /// <summary>
    /// The custom render task used as a view information source (effect will use its render buffers and rendering resolution information for particles simulation).
    /// </summary>
    API_FIELD(Attributes="NoSerialize, HideInEditor")
    ScriptingObjectReference<SceneRenderTask> CustomViewRenderTask;

public:
    /// <summary>
    /// The particles simulation update mode. Defines how to update particles emitter.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(SimulationUpdateMode.Realtime), EditorOrder(10)")
    SimulationUpdateMode UpdateMode = SimulationUpdateMode::Realtime;

    /// <summary>
    /// The fixed timestep for simulation updates. Used only if UpdateMode is set to FixedTimestep.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(1.0f / 60.0f), EditorOrder(20), VisibleIf(nameof(IsFixedTimestep))")
    float FixedTimestep = 1.0f / 60.0f;

    /// <summary>
    /// The particles simulation speed factor. Scales the particle system update delta time. Can be used to speed up or slow down the particles.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(1.0f), EditorOrder(30)")
    float SimulationSpeed = 1.0f;

    /// <summary>
    /// Determines whether the particle effect should take into account the global game time scale for simulation updates.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(true), EditorOrder(40)")
    bool UseTimeScale = true;

    /// <summary>
    /// Determines whether the particle effect should loop when it finishes playing.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(true), EditorOrder(50)")
    bool IsLooping = true;

    /// <summary>
    /// Determines whether the particle effect should play on start.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(true), EditorOrder(60)")
    bool PlayOnStart = true;

    /// <summary>
    /// If true, the particle simulation will be updated even when an actor cannot be seen by any camera. Otherwise, the simulation will stop running when the actor is off-screen.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), DefaultValue(true), EditorOrder(70)")
    bool UpdateWhenOffscreen = true;

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), EditorOrder(75), DefaultValue(DrawPass.Default)")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// The object sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Particle Effect\"), EditorOrder(80), DefaultValue(0)")
    int8 SortOrder = 0;

#if USE_EDITOR
    /// <summary>
    /// If checked, the particle emitter debug shapes will be shawn during debug drawing. This includes particle spawn location shapes display.
    /// </summary>
    API_FIELD(Attributes = "EditorDisplay(\"Particle Effect\"), EditorOrder(200)") bool ShowDebugDraw = false;
#endif

public:
    /// <summary>
    /// Gets the effect parameters collection. Those parameters are instanced from the <see cref="ParticleSystem"/> that contains a linear list of emitters and every emitter has a list of own parameters.
    /// </summary>
    API_PROPERTY() const Array<ParticleEffectParameter>& GetParameters();

    /// <summary>
    /// Gets the effect parameters collection version number. It can be used to track parameters changes that occur when particle system or one of the emitters gets reloaded/edited.
    /// </summary>
    API_PROPERTY() uint32 GetParametersVersion() const;

    /// <summary>
    /// Gets the particle parameter.
    /// </summary>
    /// <param name="emitterTrackName">The emitter track name (in particle system asset).</param>
    /// <param name="paramName">The emitter parameter name (in particle emitter asset).</param>
    /// <returns>The effect parameter or null if failed to find.</returns>
    API_FUNCTION() ParticleEffectParameter* GetParameter(const StringView& emitterTrackName, const StringView& paramName);

    /// <summary>
    /// Gets the particle parameter.
    /// </summary>
    /// <param name="emitterTrackName">The emitter track name (in particle system asset).</param>
    /// <param name="paramId">The emitter parameter ID (in particle emitter asset).</param>
    /// <returns>The effect parameter or null if failed to find.</returns>
    API_FUNCTION() ParticleEffectParameter* GetParameter(const StringView& emitterTrackName, const Guid& paramId);

    /// <summary>
    /// Gets the particle parameter value.
    /// </summary>
    /// <param name="emitterTrackName">The emitter track name (in particle system asset).</param>
    /// <param name="paramName">The emitter parameter name (in particle emitter asset).</param>
    /// <returns>The value.</returns>
    API_FUNCTION() const Variant& GetParameterValue(const StringView& emitterTrackName, const StringView& paramName);

    /// <summary>
    /// Set the particle parameter value.
    /// </summary>
    /// <param name="emitterTrackName">The emitter track name (in particle system asset).</param>
    /// <param name="paramName">The emitter parameter name (in particle emitter asset).</param>
    /// <param name="value">The value to set.</param>
    API_FUNCTION() void SetParameterValue(const StringView& emitterTrackName, const StringView& paramName, const Variant& value);

    /// <summary>
    /// Resets the particle system parameters to the default values from asset.
    /// </summary>
    API_FUNCTION() void ResetParameters();

public:
    /// <summary>
    /// Gets the current time position of the particle system timeline animation playback (in seconds).
    /// </summary>
    API_PROPERTY(Attributes="NoSerialize, HideInEditor") float GetTime() const;

    /// <summary>
    /// Sets the current time position of the particle system timeline animation playback (in seconds).
    /// </summary>
    API_PROPERTY() void SetTime(float time);

    /// <summary>
    /// Gets the last game time when particle system was updated. Value -1 indicates no previous updates.
    /// </summary>
    API_PROPERTY(Attributes="NoSerialize, HideInEditor") float GetLastUpdateTime() const;

    /// <summary>
    /// Sets the last game time when particle system was updated. Value -1 indicates no previous updates.
    /// </summary>
    API_PROPERTY() void SetLastUpdateTime(float time);

    /// <summary>
    /// Gets the particles count (total). GPU particles count is read with one frame delay (due to GPU execution).
    /// </summary>
    API_PROPERTY() int32 GetParticlesCount() const;

    /// <summary>
    /// Gets whether or not the particle effect is playing.
    /// </summary>
    API_PROPERTY(Attributes="NoSerialize, HideInEditor") bool GetIsPlaying() const;

    /// <summary>
    /// Resets the particles simulation state (clears the instance state data but preserves the instance parameters values).
    /// </summary>
    API_FUNCTION() void ResetSimulation();

    /// <summary>
    /// Performs the full particles simulation update (postponed for the next particle manager update).
    /// </summary>
    /// <param name="singleFrame">True if update animation by a single frame only (time time since last engine update), otherwise will update simulation with delta time since last update.</param>
    API_FUNCTION() void UpdateSimulation(bool singleFrame = false);

    /// <summary>
    /// Manually spawns additional particles into the simulation.
    /// </summary>
    /// <param name="count">Amount of particles to spawn.</param>
    /// <param name="emitterTrackName">Name of the emitter track to spawn particles in. Empty if spawn particles into all tracks.</param>
    API_FUNCTION() void SpawnParticles(int32 count, const StringView& emitterTrackName = String::Empty);

    /// <summary>
    /// Plays the simulation.
    /// </summary>
    API_FUNCTION() void Play();

    /// <summary>
    /// Pauses the simulation.
    /// </summary>
    API_FUNCTION() void Pause();

    /// <summary>
    /// Stops and resets the simulation.
    /// </summary>
    API_FUNCTION() void Stop();

    /// <summary>
    /// Updates the actor bounds.
    /// </summary>
    void UpdateBounds();

    /// <summary>
    /// Synchronizes this instance data with the particle system and all emitters data.
    /// </summary>
    void Sync();

    /// <summary>
    /// Gets the render task to use for particles simulation (eg. depth buffer collisions or view information).
    /// </summary>
    SceneRenderTask* GetRenderTask() const;

#if USE_EDITOR
protected:
    // Exposed parameters overrides for Editor Undo.
    API_PROPERTY(Attributes="HideInEditor, Serialize") Array<ParticleEffect::ParameterOverride>& GetParametersOverrides();
    API_PROPERTY() void SetParametersOverrides(const Array<ParticleEffect::ParameterOverride>& value);
#endif

private:
    void Update();
#if USE_EDITOR
    void UpdateExecuteInEditor();
#endif
    void CacheModifiedParameters();
    void ApplyModifiedParameters();
    void OnParticleSystemModified();
    void OnParticleSystemLoaded();
    void OnParticleEmitterLoaded();

public:
    // [Actor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
    void OnDebugDraw() override;
#endif
    void OnLayerChanged() override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif

protected:
    // [Actor]
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnActiveInTreeChanged() override;
    void OnTransformChanged() override;
};
