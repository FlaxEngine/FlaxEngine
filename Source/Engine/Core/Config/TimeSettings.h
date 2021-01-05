// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Time and game simulation settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API TimeSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(TimeSettings);
public:

    /// <summary>
    /// The target amount of the game logic updates per second (script updates frequency).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), DefaultValue(30.0f), Limit(0, 1000), EditorDisplay(\"General\", \"Update FPS\")")
    float UpdateFPS = 30.0f;

    /// <summary>
    /// The target amount of the physics simulation updates per second (also fixed updates frequency).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2), DefaultValue(60.0f), Limit(0, 1000), EditorDisplay(\"General\", \"Physics FPS\")")
    float PhysicsFPS = 60.0f;

    /// <summary>
    /// The target amount of the frames rendered per second (actual game FPS).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(3), DefaultValue(60.0f), Limit(0, 1000), EditorDisplay(\"General\", \"Draw FPS\")")
    float DrawFPS = 60.0f;

    /// <summary>
    /// The game time scale factor. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(1.0f), Limit(0, 1000.0f, 0.1f), EditorDisplay(\"General\")")
    float TimeScale = 1.0f;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the game logic update step.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(0.1f), Limit(0.1f, 1000.0f, 0.01f), EditorDisplay(\"General\")")
    float MaxUpdateDeltaTime = 0.1f;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static TimeSettings* Get();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
