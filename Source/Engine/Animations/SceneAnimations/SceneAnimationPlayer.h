// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Level/Actors/PostFxVolume.h"
#include "SceneAnimation.h"

/// <summary>
/// The scene animation playback actor.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Animation/Scene Animation\"), ActorToolbox(\"Other\", \"Scene Animation\")")
class FLAXENGINE_API SceneAnimationPlayer : public Actor, public IPostFxSettingsProvider
{
    DECLARE_SCENE_OBJECT(SceneAnimationPlayer);

    /// <summary>
    /// Describes the scene animation updates frequency.
    /// </summary>
    API_ENUM() enum class UpdateModes
    {
        /// <summary>
        /// Animation will be updated every game logic update.
        /// </summary>
        EveryUpdate = 0,

        /// <summary>
        /// Animation can be updated manually by the user scripts.
        /// </summary>
        Manual = 1,
    };

private:
    enum class PlayState
    {
        Stopped,
        Paused,
        Playing,
    };

    struct TrackInstance
    {
        ScriptingObjectReference<ScriptingObject> Object;
        MObject* ManagedObject = nullptr;
        MProperty* Property = nullptr;
        MField* Field = nullptr;
        MMethod* Method = nullptr;
        int32 RestoreStateIndex = -1;
        bool Warn = true;

        TrackInstance()
        {
        }
    };

    float _time = 0.0f;
    float _lastTime = 0.0f;
    PlayState _state = PlayState::Stopped;
    Array<TrackInstance> _tracks;
    Array<byte> _tracksDataStack;
    Array<Actor*> _subActors;
    Array<byte> _restoreData;
    Camera* _cameraCutCam = nullptr;
    bool _isUsingCameraCuts = false;
    Dictionary<Guid, Guid> _objectsMapping;

    // PostFx settings to use
    struct
    {
        CameraArtifactsSettings CameraArtifacts;
        PostFxMaterialsSettings PostFxMaterials;
    } _postFxSettings;

public:
    /// <summary>
    /// The scene animation to play.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(0), DefaultValue(null)")
    AssetReference<SceneAnimation> Animation;

    /// <summary>
    /// The animation playback speed factor. Scales the timeline update delta time. Can be used to speed up or slow down the sequence.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(10), DefaultValue(1.0f)")
    float Speed = 1.0f;

    /// <summary>
    /// The animation start time. Can be used to skip part of the sequence on begin.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(20), DefaultValue(0.0f)")
    float StartTime = 0.0f;

    /// <summary>
    /// Determines whether the scene animation should take into account the global game time scale for simulation updates.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(30), DefaultValue(true)")
    bool UseTimeScale = true;

    /// <summary>
    /// Determines whether the scene animation should loop when it finishes playing.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(40), DefaultValue(false)")
    bool Loop = false;

    /// <summary>
    /// Determines whether the scene animation should auto play on game start.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\", \"Play On Start\"), EditorOrder(50), DefaultValue(false)")
    bool PlayOnStart = false;

    /// <summary>
    /// Determines whether the scene animation should randomize the start time on play begin.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(60), DefaultValue(false)")
    bool RandomStartTime = false;

    /// <summary>
    /// Determines whether the scene animation should restore initial state on playback stop. State is cached when animation track starts play after being stopped (not paused).
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\", \"Restore State On Stop\"), EditorOrder(70), DefaultValue(false)")
    bool RestoreStateOnStop = false;

    /// <summary>
    /// The animation update mode.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(80), DefaultValue(UpdateModes.EveryUpdate)")
    UpdateModes UpdateMode = UpdateModes::EveryUpdate;

    /// <summary>
    /// Determines whether the scene animation should automatically map prefab objects from scene animation into prefab instances. Useful for reusable animations to automatically link prefab objects.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Scene Animation\"), EditorOrder(100)")
    bool UsePrefabObjects = false;

public:
    /// <summary>
    /// Gets the value that determines whether the scene animation is playing.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsPlaying() const
    {
        return _state == PlayState::Playing;
    }

    /// <summary>
    /// Gets the value that determines whether the scene animation is paused.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsPaused() const
    {
        return _state == PlayState::Paused;
    }

    /// <summary>
    /// Gets the value that determines whether the scene animation is stopped.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsStopped() const
    {
        return _state == PlayState::Stopped;
    }

    /// <summary>
    /// Gets the current animation playback time position (seconds).
    /// </summary>
    /// <returns>The animation playback time position (seconds).</returns>
    API_PROPERTY(Attributes="NoSerialize, HideInEditor") float GetTime() const;

    /// <summary>
    /// Sets the current animation playback time position (seconds).
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetTime(float value);

    /// <summary>
    /// Starts playing the animation. Has no effect if animation is already playing.
    /// </summary>
    API_FUNCTION() void Play();

    /// <summary>
    /// Pauses the animation. Has no effect if animation is not playing.
    /// </summary>
    API_FUNCTION() void Pause();

    /// <summary>
    /// Stops playing the animation. Has no effect if animation is already stopped.
    /// </summary>
    API_FUNCTION() void Stop();

    /// <summary>
    /// Manually ticks the animation by performing the playback update with a given time delta.
    /// </summary>
    /// <param name="dt">The update delta time (in seconds). It does not get scaled by player Speed parameter.</param>
    API_FUNCTION() void Tick(float dt);

    /// <summary>
    /// Adds an object mapping. The object `from` represented by it's unique ID will be redirected to the specified `to`. Can be used to reuse the same animation for different objects.
    /// </summary>
    /// <param name="from">The source object from the scene animation asset to replace.</param>
    /// <param name="to">The destination object to animate.</param>
    API_FUNCTION() void MapObject(const Guid& from, const Guid& to);

    /// <summary>
    /// Adds an object mapping for the object track. The track name `from` will be redirected to the specified object `to`. Can be used to reuse the same animation for different objects.
    /// </summary>
    /// <param name="from">The source track name from the scene animation asset to replace.</param>
    /// <param name="to">The destination object to animate.</param>
    API_FUNCTION() void MapTrack(const StringView& from, const Guid& to);

private:
    void Restore(SceneAnimation* anim, int32 stateIndexOffset);
    bool TickPropertyTrack(int32 trackIndex, int32 stateIndexOffset, SceneAnimation* anim, float time, const SceneAnimation::Track& track, TrackInstance& state, void* target);
    typedef Array<SceneAnimation*, FixedAllocation<8>> CallStack;
    void Tick(SceneAnimation* anim, float time, float dt, int32 stateIndexOffset, CallStack& callStack);
    void Tick();
    void OnAnimationModified();
    void ResetState();

public:
    // [Actor]
    bool HasContentLoaded() const override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif

    // [IPostFxSettingsProvider]
    void Collect(RenderContext& renderContext) override;
    void Blend(PostProcessSettings& other, float weight) override;

protected:
    // [Actor]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
