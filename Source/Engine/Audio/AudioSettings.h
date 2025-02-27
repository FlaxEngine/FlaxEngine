// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Audio settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API AudioSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AudioSettings);
    API_AUTO_SERIALIZATION();

public:
    /// <summary>
    /// If checked, audio playback will be disabled in build game. Can be used if game uses custom audio playback engine.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    bool DisableAudio = false;

    /// <summary>
    /// The doppler effect factor. Scale for source and listener velocities. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"General\")")
    float DopplerFactor = 1.0f;

    /// <summary>
    /// True if mute all audio playback when game has no use focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(200), EditorDisplay(\"General\", \"Mute On Focus Loss\")")
    bool MuteOnFocusLoss = true;

    /// <summary>
    /// Enables or disables HRTF audio for in-engine processing of 3D audio (if supported by platform).
    /// If enabled, the user should be using two-channel/headphones audio output and have all other surround virtualization disabled (Atmos, DTS:X, vendor specific, etc.)
    /// </summary>
    API_FIELD(Attributes="EditorOrder(300), EditorDisplay(\"Spatial Audio\")")
    bool EnableHRTF = false;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static AudioSettings* Get();

    // [SettingsBase]
    void Apply() override;
};
