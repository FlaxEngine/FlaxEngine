// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

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
    VideoBackend* Backend = nullptr;
    GPUTexture* Frame = nullptr;
    GPUBuffer* FrameUpload = nullptr;
    int32 Width = 0, Height = 0, AvgBitRate = 0, FramesCount = 0;
    int32 VideoFrameWidth = 0, VideoFrameHeight = 0;
    PixelFormat Format = PixelFormat::Unknown;
    float FrameRate = 0.0f;
    TimeSpan Duration = TimeSpan(0);
    TimeSpan VideoFrameTime = TimeSpan(0), VideoFrameDuration = TimeSpan(0);
    AudioDataInfo AudioInfo = {};
    BytesContainer VideoFrameMemory;
    class GPUUploadVideoFrameTask* UploadVideoFrameTask = nullptr;
    uintptr BackendState[8] = {};

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

    void InitVideoFrame();
    void UpdateVideoFrame(Span<byte> frame, TimeSpan time, TimeSpan duration);
    void ReleaseResources();
};
