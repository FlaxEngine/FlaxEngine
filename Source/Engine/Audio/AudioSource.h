// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Content/AssetReference.h"
#include "AudioClip.h"

/// <summary>
/// Represents a source for emitting audio. Audio can be played spatially (gun shot), or normally (music). Each audio source must have an AudioClip to play - back, and it can also have a position in the case of spatial (3D) audio.
/// </summary>
/// <remarks>
/// Whether or not an audio source is spatial is controlled by the assigned AudioClip.The volume and the pitch of a spatial audio source is controlled by its position and the AudioListener's position/direction/velocity.
/// </remarks>
API_CLASS(Attributes="ActorContextMenu(\"New/Audio/Audio Source\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API AudioSource : public Actor
{
    DECLARE_SCENE_OBJECT(AudioSource);
    friend class AudioStreamingHandler;
    friend class AudioClip;

public:
    /// <summary>
    /// Valid states in which AudioSource can be in.
    /// </summary>
    API_ENUM() enum class States
    {
        /// <summary>
        /// The source is currently playing.
        /// </summary>
        Playing = 0,

        /// <summary>
        /// The source is currently paused (play will resume from paused point).
        /// </summary>
        Paused = 1,

        /// <summary>
        /// The source is currently stopped (play will resume from start).
        /// </summary>
        Stopped = 2
    };

private:
    Vector3 _velocity;
    Vector3 _prevPos;
    float _volume;
    float _pitch;
    float _pan = 0.0f;
    float _minDistance;
    float _attenuation = 1.0f;
    float _dopplerFactor = 1.0f;
    bool _loop;
    bool _playOnStart;
    float _startTime;
    bool _allowSpatialization;

    bool _isActuallyPlayingSth = false;
    bool _startingToPlay = false;
    bool _needToUpdateStreamingBuffers = false;
    States _state = States::Stopped;

    States _savedState = States::Stopped;
    float _savedTime = 0;
    int32 _streamingFirstChunk = 0;

public:
    /// <summary>
    /// The internal ID of this audio source used by the audio backend. Empty if 0.
    /// </summary>
    uint32 SourceID = 0;

    /// <summary>
    /// The audio clip asset used as a source of the sound.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(null), EditorDisplay(\"Audio Source\")")
    AssetReference<AudioClip> Clip;

    /// <summary>
    /// Gets the velocity of the source. Determines pitch in relation to AudioListener's position. Only relevant for spatial (3D) sources.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const Vector3& GetVelocity() const
    {
        return _velocity;
    }

    /// <summary>
    /// Gets the volume of the audio played from this source, in [0, 1] range.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(1.0f), Limit(0, 1, 0.01f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetVolume() const
    {
        return _volume;
    }

    /// <summary>
    /// Sets the volume of the audio played from this source, in [0, 1] range.
    /// </summary>
    API_PROPERTY() void SetVolume(float value);

    /// <summary>
    /// Gets the pitch of the played audio. The default is 1.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(1.0f), Limit(0.5f, 2.0f, 0.01f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetPitch() const
    {
        return _pitch;
    }

    /// <summary>
    /// Sets the pitch of the played audio. The default is 1.
    /// </summary>
    API_PROPERTY() void SetPitch(float value);

    /// <summary>
    /// Gets the stereo pan of the played audio (-1 is left speaker, 1 is right speaker, 0 is balanced). The default is 1. Used by non-spatial audio only.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(0.0f), Limit(-1.0f, 1.0f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetPan() const
    {
        return _pan;
    }

    /// <summary>
    /// Sets the stereo pan of the played audio (-1 is left speaker, 1 is right speaker, 0 is balanced). The default is 0. Used by non-spatial audio only.
    /// </summary>
    API_PROPERTY() void SetPan(float value);

    /// <summary>
    /// Determines whether the audio clip should loop when it finishes playing.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE bool GetIsLooping() const
    {
        return _loop;
    }

    /// <summary>
    /// Determines whether the audio clip should loop when it finishes playing.
    /// </summary>
    API_PROPERTY() void SetIsLooping(bool value);

    /// <summary>
    /// Determines whether the audio clip should autoplay on level start.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(false), EditorDisplay(\"Audio Source\", \"Play On Start\")")
    FORCE_INLINE bool GetPlayOnStart() const
    {
        return _playOnStart;
    }

    /// <summary>
    /// Determines the time (in seconds) at which the audio clip starts playing if Play On Start is enabled.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(51), DefaultValue(0.0f), Limit(0, float.MaxValue, 0.01f), EditorDisplay(\"Audio Source\", \"Start Time\"), VisibleIf(nameof(PlayOnStart))")
    FORCE_INLINE float GetStartTime() const
    {
        return _startTime;
    }

    /// <summary>
    /// Determines whether the audio clip should autoplay on game start.
    /// </summary>
    API_PROPERTY() void SetPlayOnStart(bool value);

    /// <summary>
    /// Determines the time (in seconds) at which the audio clip starts playing if Play On Start is enabled.
    /// </summary>
    API_PROPERTY() void SetStartTime(float value);

    /// <summary>
    /// Gets the minimum distance at which audio attenuation starts. When the listener is closer to the source than this value, audio is heard at full volume. Once farther away the audio starts attenuating.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(1000.0f), Limit(0, float.MaxValue, 0.1f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetMinDistance() const
    {
        return _minDistance;
    }

    /// <summary>
    /// Sets the minimum distance at which audio attenuation starts. When the listener is closer to the source than this value, audio is heard at full volume. Once farther away the audio starts attenuating.
    /// </summary>
    API_PROPERTY() void SetMinDistance(float value);

    /// <summary>
    /// Gets the attenuation that controls how quickly does audio volume drop off as the listener moves further from the source.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(70), DefaultValue(1.0f), Limit(0, float.MaxValue, 0.1f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetAttenuation() const
    {
        return _attenuation;
    }

    /// <summary>
    /// Sets the attenuation that controls how quickly does audio volume drop off as the listener moves further from the source. At 0, no distance attenuation ever occurs.
    /// </summary>
    API_PROPERTY() void SetAttenuation(float value);

    /// <summary>
    /// Gets the doppler effect factor. Scale for source velocity. Default is 1.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(75), DefaultValue(1.0f), Limit(0, float.MaxValue, 0.1f), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE float GetDopplerFactor() const
    {
        return _dopplerFactor;
    }

    /// <summary>
    /// Sets the doppler effect factor. Scale for source velocity. Default is 1.
    /// </summary>
    API_PROPERTY() void SetDopplerFactor(float value);

    /// <summary>
    /// If checked, source can play spatial 3d audio (when audio clip supports it), otherwise will always play as 2d sound.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(80), DefaultValue(true), EditorDisplay(\"Audio Source\")")
    FORCE_INLINE bool GetAllowSpatialization() const
    {
        return _allowSpatialization;
    }

    /// <summary>
    /// If checked, source can play spatial 3d audio (when audio clip supports it), otherwise will always play as 2d sound.
    /// </summary>
    API_PROPERTY() void SetAllowSpatialization(bool value);

public:
    /// <summary>
    /// Starts playing the currently assigned audio clip.
    /// </summary>
    API_FUNCTION() void Play();

    /// <summary>
    /// Pauses the audio playback.
    /// </summary>
    API_FUNCTION() void Pause();

    /// <summary>
    /// Stops audio playback, rewinding it to the start.
    /// </summary>
    API_FUNCTION() void Stop();

    /// <summary>
    /// Gets the current state of the audio playback (playing/paused/stopped).
    /// </summary>
    API_PROPERTY() FORCE_INLINE AudioSource::States GetState() const
    {
        return _state;
    }

    /// <summary>
    /// Gets the current time of playback. If playback has not yet started, it specifies the time at which playback will start at. The time is in seconds, in range [0, ClipLength].
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") float GetTime() const;

    /// <summary>
    /// Sets the current time of playback. If playback has not yet started, it specifies the time at which playback will start at. The time is in seconds, in range [0, ClipLength].
    /// </summary>
    /// <param name="time">The time.</param>
    API_PROPERTY() void SetTime(float time);

    /// <summary>
    /// Returns true if the sound source is three-dimensional (volume and pitch varies based on listener distance and velocity).
    /// </summary>
    API_PROPERTY() bool Is3D() const;

    /// <summary>
    /// Returns true if audio clip is valid, loaded and uses dynamic data streaming.
    /// </summary>
    API_PROPERTY() bool UseStreaming() const;

public:
    /// <summary>
    /// Determines whether this audio source started playing audio via audio backend. After audio play it may wait for audio clip data to be loaded or streamed.
    /// [Deprecated in v1.9]
    /// </summary>
    API_PROPERTY() DEPRECATED("Use IsActuallyPlaying instead.") FORCE_INLINE bool IsActuallyPlayingSth() const
    {
        return _isActuallyPlayingSth;
    }

    /// <summary>
    /// Determines whether this audio source started playing audio via audio backend. After audio play it may wait for audio clip data to be loaded or streamed.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsActuallyPlaying() const
    {
        return _isActuallyPlayingSth;
    }

    /// <summary>
    /// Requests the audio streaming buffers update. Rises tha flag to synchronize audio backend buffers of the emitter during next game logic update.
    /// </summary>
    void RequestStreamingBuffersUpdate();

private:
    void OnClipChanged();
    void OnClipLoaded();

    /// <summary>
    /// Plays the audio source. Should have buffer(s) binded before.
    /// </summary>
    void PlayInternal();

    void Update();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool HasContentLoaded() const override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
    void BeginPlay(SceneBeginData* data) override;
};
