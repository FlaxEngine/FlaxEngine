// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// Audio settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API AudioSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(AudioSettings);
public:

    /// <summary>
    /// If checked, audio playback will be disabled in build game. Can be used if game uses custom audio playback engine.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), DefaultValue(false), EditorDisplay(\"General\")")
    bool DisableAudio = false;

    /// <summary>
    /// The doppler effect factor. Scale for source and listener velocities. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), DefaultValue(1.0f), EditorDisplay(\"General\")")
    float DopplerFactor = 1.0f;

    /// <summary>
    /// True if mute all audio playback when game has no use focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(200), DefaultValue(true), EditorDisplay(\"General\", \"Mute On Focus Loss\")")
    bool MuteOnFocusLoss = true;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static AudioSettings* Get();

    // [SettingsBase]
    void Apply() override;

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(DisableAudio);
        DESERIALIZE(DopplerFactor);
        DESERIALIZE(MuteOnFocusLoss);
    }
};
