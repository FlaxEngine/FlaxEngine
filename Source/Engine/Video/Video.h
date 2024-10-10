// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

/// <summary>
/// The video service used for video media playback.
/// </summary>
class Video
{
public:
    static class TaskGraphSystem* System;
    static bool CreatePlayerBackend(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player);
};
