// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Level/Actor.h"
#include "Engine/Content/AssetReference.h"
#include "Types.h"

/// <summary>
/// Video playback utility. Video content can be presented in UI (via VideoBrush), used in materials (via texture parameter bind) or used manually in shaders.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Video Player\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API VideoPlayer : public Actor
{
    DECLARE_SCENE_OBJECT(VideoPlayer);
    API_AUTO_SERIALIZATION();

public:
    /// <summary>
    /// Valid states in which VideoPlayer can be in.
    /// </summary>
    API_ENUM() enum class States
    {
        /// <summary>
        /// The video is currently stopped (play will resume from start).
        /// </summary>
        Stopped = 0,

        /// <summary>
        /// The video is currently playing.
        /// </summary>
        Playing = 1,

        /// <summary>
        /// The video is currently paused (play will resume from paused point).
        /// </summary>
        Paused = 2,
    };

private:
    VideoBackendPlayer _player;
    States _state = States::Stopped;
    bool _loop = false, _isSpatial = false;
    float _volume = 1.0f, _pan = 0.0f, _minDistance = 1000.0f, _attenuation = 1.0f;

public:
    ~VideoPlayer();

    /// <summary>
    /// The video clip Url path used as a source of the media. Can be local file (absolute or relative path), or streamed resource ('http://').
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(\"\"), EditorDisplay(\"Video Player\"), AssetReference(\".mp4\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.FilePathEditor\")")
    String Url;

    /// <summary>
    /// Determines whether the video clip should loop when it finishes playing.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(false), EditorDisplay(\"Video Player\")")
    FORCE_INLINE bool GetIsLooping() const
    {
        return _loop;
    }

    /// <summary>
    /// Determines whether the video clip should loop when it finishes playing.
    /// </summary>
    API_PROPERTY() void SetIsLooping(bool value);

    /// <summary>
    /// Determines whether the video clip should autoplay on level start.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(false), EditorDisplay(\"Video Player\", \"Play On Start\")")
    bool PlayOnStart = false;

    /// <summary>
    /// Determines the time (in seconds) at which the video clip starts playing if Play On Start is enabled.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(35), DefaultValue(0.0f), Limit(0, float.MaxValue, 0.01f), EditorDisplay(\"Video Player\"), VisibleIf(nameof(PlayOnStart))")
    float StartTime = 0.0f;

    /// <summary>
    /// If checked, video player us using spatialization to play 3d audio, otherwise will always play as 2d sound.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(false), EditorDisplay(\"Video Player\")")
    FORCE_INLINE bool GetIsAudioSpatial() const
    {
        return _isSpatial;
    }

    /// <summary>
    /// If checked, source can play spatial 3d audio (when audio clip supports it), otherwise will always play as 2d sound. At 0, no distance attenuation ever occurs.
    /// </summary>
    API_PROPERTY() void SetIsAudioSpatial(bool value);

    /// <summary>
    /// Gets the volume of the audio played from this video, in [0, 1] range.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(1.0f), Limit(0, 1, 0.01f), EditorDisplay(\"Video Player\")")
    FORCE_INLINE float GetAudioVolume() const
    {
        return _volume;
    }

    /// <summary>
    /// Sets the volume of the audio played from this video, in [0, 1] range.
    /// </summary>
    API_PROPERTY() void SetAudioVolume(float value);

    /// <summary>
    /// Gets the stereo pan of the played audio (-1 is left speaker, 1 is right speaker, 0 is balanced). The default is 0. Used by non-spatial audio only.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(0.0f), Limit(-1.0f, 1.0f), EditorDisplay(\"Video Player\"), VisibleIf(nameof(IsAudioSpatial), true)")
    FORCE_INLINE float GetAudioPan() const
    {
        return _pan;
    }

    /// <summary>
    /// Sets the stereo pan of the played audio (-1 is left speaker, 1 is right speaker, 0 is balanced). The default is 0. Used by non-spatial audio only.
    /// </summary>
    API_PROPERTY() void SetAudioPan(float value);

    /// <summary>
    /// Gets the minimum distance at which audio attenuation starts. When the listener is closer to the video player than this value, audio is heard at full volume. Once farther away the audio starts attenuating. Used by spatial audio only.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(120), DefaultValue(1000.0f), Limit(0, float.MaxValue, 0.1f), EditorDisplay(\"Video Player\"), VisibleIf(nameof(IsAudioSpatial))")
    FORCE_INLINE float GetAudioMinDistance() const
    {
        return _minDistance;
    }

    /// <summary>
    /// Sets the minimum distance at which audio attenuation starts. When the listener is closer to the video player than this value, audio is heard at full volume. Once farther away the audio starts attenuating. Used by spatial audio only.
    /// </summary>
    API_PROPERTY() void SetAudioMinDistance(float value);

    /// <summary>
    /// Gets the attenuation that controls how quickly does audio volume drop off as the listener moves further from the video player. Used by spatial audio only.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(130), DefaultValue(1.0f), Limit(0, float.MaxValue, 0.1f), EditorDisplay(\"Video Player\"), VisibleIf(nameof(IsAudioSpatial))")
    FORCE_INLINE float GetAudioAttenuation() const
    {
        return _attenuation;
    }

    /// <summary>
    /// Sets the attenuation that controls how quickly does audio volume drop off as the listener moves further from the video player. At 0, no distance attenuation ever occurs. Used by spatial audio only.
    /// </summary>
    API_PROPERTY() void SetAudioAttenuation(float value);

public:
    /// <summary>
    /// Starts playing the currently assigned video Url.
    /// </summary>
    API_FUNCTION() void Play();

    /// <summary>
    /// Pauses the video playback.
    /// </summary>
    API_FUNCTION() void Pause();

    /// <summary>
    /// Stops video playback, rewinding it to the start.
    /// </summary>
    API_FUNCTION() void Stop();

    /// <summary>
    /// Gets the current state of the video playback (playing/paused/stopped).
    /// </summary>
    API_PROPERTY() FORCE_INLINE VideoPlayer::States GetState() const
    {
        return _state;
    }

    /// <summary>
    /// Gets the current time of playback. The time is in seconds, in range [0, Duration].
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") float GetTime() const;

    /// <summary>
    /// Sets the current time of playback. The time is in seconds, in range [0, Duration].
    /// </summary>
    /// <param name="time">The time.</param>
    API_PROPERTY() void SetTime(float time);

    /// <summary>
    /// Gets the media duration of playback (in seconds). Valid only when media was loaded by the video backend.
    /// </summary>
    API_PROPERTY() float GetDuration() const;

    /// <summary>
    /// Gets the media frame rate of playback (amount of frames to be played per second). Valid only when media was loaded by the video backend.
    /// </summary>
    API_PROPERTY() float GetFrameRate() const;

    /// <summary>
    /// Gets the amount of video frames decoded and send to GPU during playback. Can be used to detect if video has started playback with any visible changes (for video frame texture contents). Valid only when media was loaded by the video backend.
    /// </summary>
    API_PROPERTY() int32 GetFramesCount() const;

    /// <summary>
    /// Gets the video frame dimensions (in pixels). Valid only when media was loaded by the video backend.
    /// </summary>
    API_PROPERTY() Int2 GetSize() const;

    /// <summary>
    /// Gets the video frame texture (GPU resource). Created on the playback start. Can be binded to materials and shaders to display the video image.
    /// </summary>
    API_PROPERTY() GPUTexture* GetFrame() const;

private:
    void GetInfo(VideoBackendPlayerInfo& info) const;
    void UpdateInfo();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }

    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
    void BeginPlay(SceneBeginData* data) override;
};
