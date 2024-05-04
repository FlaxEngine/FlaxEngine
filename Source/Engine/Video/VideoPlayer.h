// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    bool _loop = false;

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
    /// Determines whether the video clip should auto play on level start.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(false), EditorDisplay(\"Video Player\", \"Play On Start\")")
    bool PlayOnStart = false;

    /// <summary>
    /// Determines the time (in seconds) at which the video clip starts playing if Play On Start is enabled.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(35), DefaultValue(0.0f), Limit(0, float.MaxValue, 0.01f), EditorDisplay(\"Video Player\"), VisibleIf(nameof(PlayOnStart))")
    float StartTime = 0.0f;

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
    /// Gets the current time of playback. If playback has not yet started, it specifies the time at which playback will start at. The time is in seconds, in range [0, Duration].
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") float GetTime() const;

    /// <summary>
    /// Sets the current time of playback. If playback has not yet started, it specifies the time at which playback will start at. The time is in seconds, in range [0, Duration].
    /// </summary>
    /// <param name="time">The time.</param>
    API_PROPERTY() void SetTime(float time);

    /// <summary>
    /// Gets the media duration of playback (in seconds).
    /// </summary>
    API_PROPERTY() float GetDuration() const;

    /// <summary>
    /// Gets the media frame rate of playback (amount of frames to be played per second).
    /// </summary>
    API_PROPERTY() float GetFrameRate() const;

    /// <summary>
    /// Gets the amount of video frames decoded and send to GPU during playback. Can be used to detect if video has started playback with any visible changes (for video frame texture contents).
    /// </summary>
    API_PROPERTY() int32 GetFramesCount() const;

    /// <summary>
    /// Gets the video frame dimensions (in pixels).
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
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
    void BeginPlay(SceneBeginData* data) override;
};
