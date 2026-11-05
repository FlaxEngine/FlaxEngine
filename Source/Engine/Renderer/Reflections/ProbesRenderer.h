// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/TimeSpan.h"

class Actor;

/// <summary>
/// Probes rendering service
/// </summary>
class ProbesRenderer
{
public:
    /// <summary>
    /// Time delay between probe updates. Can be used to improve performance by rendering probes less often.
    /// </summary>
    static TimeSpan UpdateDelay;

    /// <summary>
    /// Timeout after the last probe rendered when resources used to render it should be released.
    /// </summary>
    static TimeSpan ReleaseTimeout;

    /// <summary>
    /// Maximum amount of cubemap faces or filtering passes that can be performed per-frame (in total). Set it to 7 to perform whole cubemap capture within a single frame, lower values spread the work across multiple frames.
    /// </summary>
    static int32 MaxWorkPerFrame;

    static Delegate<Actor*> OnRegisterBake;

    static Delegate<Actor*> OnFinishBake;

public:
    /// <summary>
    /// Register probe to baking service.
    /// </summary>
    /// <param name="probe">Probe to bake</param>
    /// <param name="timeout">Timeout in seconds left to bake it.</param>
    static void Bake(class EnvironmentProbe* probe, float timeout = 0);

    /// <summary>
    /// Register probe to baking service.
    /// </summary>
    /// <param name="probe">Probe to bake</param>
    /// <param name="timeout">Timeout in seconds left to bake it.</param>
    static void Bake(class SkyLight* probe, float timeout = 0);
};
