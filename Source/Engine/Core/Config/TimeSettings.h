// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Contains available time update modes for update and draw calls.
/// </summary>
API_ENUM(Namespace="FlaxEditor.Content.Settings") enum class FLAXENGINE_API TimeUpdateMode
{
    /// <summary>
    /// Update and draw run separately - i.e. can run at different FPS, might get called at the same frame but this is not guaranteed.
    /// Not recommended, might cause visible stutter and other graphical glitches when update is running less often than draw.
    /// </summary>
    UpdateAndDrawSeparated,
    
    /// <summary>
    /// Update and draw run together. Update is called every time draw is also going to be called.
    /// </summary>
    UpdateAndDrawPaired
};

/// <summary>
/// Time and game simulation settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API TimeSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(TimeSettings);
public:

    /// <summary>
    /// The mode at which update and draw is called. Does not affect fixed update/physics fps.
    /// When set to UpdateAndDrawPaired, only UpdateFPS and PhysicsFPS properties are in use, DrawFPS does not affect timing.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\", \"Update mode\")")
    TimeUpdateMode UpdateMode = TimeUpdateMode::UpdateAndDrawPaired;

    /// <summary>
    /// The target amount of the game logic updates per second (script updates frequency).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), Limit(0, 1000), EditorDisplay(\"General\", \"Update FPS\")")
    float UpdateFPS = 60.0f;

    /// <summary>
    /// The target amount of the physics simulation updates per second (also fixed updates frequency).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2), Limit(0, 1000), EditorDisplay(\"General\", \"Physics FPS\")")
    float PhysicsFPS = 60.0f;

    /// <summary>
    /// The target amount of the frames rendered per second (actual game FPS).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(3), Limit(0, 1000), EditorDisplay(\"General\", \"Draw FPS\"), VisibleIf(\"IsDrawFPSUsed\")")
    float DrawFPS = 60.0f;

    /// <summary>
    /// The game time scale factor. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), Limit(0, 1000.0f, 0.1f), EditorDisplay(\"General\")")
    float TimeScale = 1.0f;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the game logic update step.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), Limit(0.1f, 1000.0f, 0.01f), EditorDisplay(\"General\")")
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
