// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Audio/Types.h"
#include "Engine/Graphics/PixelFormat.h"

class Video;
class VideoPlayer;
class VideoBackend;
struct VideoBackendPlayer;
struct VideoBackendPlayerInfo;
class GPUTexture;
class GPUBuffer;
class GPUPipelineState;

/// <summary>
/// Video player instance created by backend.
/// </summary>
struct VideoBackendPlayer
{
    VideoBackend* Backend;
    GPUTexture* Frame;
    GPUBuffer* FrameUpload;
    class GPUUploadVideoFrameTask* UploadVideoFrameTask;
    const Transform* Transform;
#ifdef TRACY_ENABLE
    Char* DebugUrl;
    int32 DebugUrlLen;
#endif
    int32 Width, Height, FramesCount;
    int32 VideoFrameWidth, VideoFrameHeight;
    PixelFormat Format;
    float FrameRate;
    float AudioVolume;
    float AudioPan;
    float AudioMinDistance;
    float AudioAttenuation;
    uint8 IsAudioSpatial : 1;
    uint8 IsAudioPlayPending : 1;
    TimeSpan Duration;
    TimeSpan VideoFrameTime, VideoFrameDuration;
    TimeSpan AudioBufferTime, AudioBufferDuration;
    AudioDataInfo AudioInfo;
    BytesContainer VideoFrameMemory;
    uint32 AudioSource;
    uint32 NextAudioBuffer;
    uint32 AudioBuffers[30];
    uintptr BackendState[8];

    VideoBackendPlayer()
    {
        Platform::MemoryClear(this, sizeof(VideoBackendPlayer));
    }

    POD_COPYABLE(VideoBackendPlayer);

    template<typename T>
    FORCE_INLINE T& GetBackendState()
    {
        static_assert(sizeof(T) <= sizeof(BackendState), "Increase state data to fit per-backend storage.");
        return *(T*)BackendState;
    }

    template<typename T>
    FORCE_INLINE const T& GetBackendState() const
    {
        static_assert(sizeof(T) <= sizeof(BackendState), "Increase state data to fit per-backend storage.");
        return *(const T*)BackendState;
    }

    void Created(const VideoBackendPlayerInfo& info);
    void Updated(const VideoBackendPlayerInfo& info);
    void PlayAudio();
    void PauseAudio();
    void StopAudio();
    void InitVideoFrame();
    void UpdateVideoFrame(Span<byte> data, TimeSpan time, TimeSpan duration);
    void UpdateAudioBuffer(Span<byte> data, TimeSpan time, TimeSpan duration);
    void Tick();
    void ReleaseResources();
};
