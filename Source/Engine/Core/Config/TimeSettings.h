// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// Time and game simulation settings container.
/// </summary>
class TimeSettings : public SettingsBase<TimeSettings>
{
public:

    /// <summary>
    /// The target amount of the game logic updates per second (script updates frequency).
    /// </summary>
    float UpdateFPS = 30.0f;

    /// <summary>
    /// The target amount of the physics simulation updates per second (also fixed updates frequency).
    /// </summary>
    float PhysicsFPS = 60.0f;

    /// <summary>
    /// The target amount of the frames rendered per second (actual game FPS).
    /// </summary>
    float DrawFPS = 60.0f;

    /// <summary>
    /// The game time scale factor. Default is 1.
    /// </summary>
    float TimeScale = 1.0f;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the game logic update step.
    /// </summary>
    float MaxUpdateDeltaTime = (1.0f / 10.0f);

public:

    // [SettingsBase]
    void Apply() override;

    void RestoreDefault() override
    {
        UpdateFPS = 30.0f;
        PhysicsFPS = 60.0f;
        DrawFPS = 60.0f;
        TimeScale = 1.0f;
        MaxUpdateDeltaTime = 1.0f / 10.0f;
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(UpdateFPS);
        DESERIALIZE(PhysicsFPS);
        DESERIALIZE(DrawFPS);
        DESERIALIZE(TimeScale);
        DESERIALIZE(MaxUpdateDeltaTime);
    }
};
