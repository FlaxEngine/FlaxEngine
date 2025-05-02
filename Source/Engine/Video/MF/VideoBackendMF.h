// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if VIDEO_API_MF

#include "../VideoBackend.h"

/// <summary>
/// The Media Foundation video backend.
/// </summary>
class VideoBackendMF : public VideoBackend
{
public:
    // [VideoBackend]
    bool Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player) override;
    void Player_Destroy(VideoBackendPlayer& player) override;
    void Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info) override;
    void Player_Play(VideoBackendPlayer& player) override;
    void Player_Pause(VideoBackendPlayer& player) override;
    void Player_Stop(VideoBackendPlayer& player) override;
    void Player_Seek(VideoBackendPlayer& player, TimeSpan time) override;
    TimeSpan Player_GetTime(const VideoBackendPlayer& player) override;
    const Char* Base_Name() override;
    bool Base_Init() override;
    void Base_Update(TaskGraph* graph) override;
    void Base_Dispose() override;
};

#endif
