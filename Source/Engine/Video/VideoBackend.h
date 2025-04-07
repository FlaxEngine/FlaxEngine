// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/StringView.h"

/// <summary>
/// Description for new video player creation by backend.
/// </summary>
struct VideoBackendPlayerInfo
{
    StringView Url;
    bool Loop;
    bool Spatial;
    float Volume;
    float Pan;
    float MinDistance;
    float Attenuation;
    const Transform* Transform;
};

/// <summary>
/// The helper class for that handles active Video backend operations.
/// </summary>
class VideoBackend
{
public:
    virtual ~VideoBackend()
    {
    }

    // Player
    virtual bool Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player) = 0;
    virtual void Player_Destroy(VideoBackendPlayer& player) = 0;
    virtual void Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info) = 0;
    virtual void Player_Play(VideoBackendPlayer& player) = 0;
    virtual void Player_Pause(VideoBackendPlayer& player) = 0;
    virtual void Player_Stop(VideoBackendPlayer& player) = 0;
    virtual void Player_Seek(VideoBackendPlayer& player, TimeSpan time) = 0;
    virtual TimeSpan Player_GetTime(const VideoBackendPlayer& player) = 0;

    // Base
    virtual const Char* Base_Name() = 0;
    virtual bool Base_Init() = 0;
    virtual void Base_Update(class TaskGraph* graph) = 0;
    virtual void Base_Dispose() = 0;
};
