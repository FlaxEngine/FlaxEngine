// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// Audio settings container.
/// </summary>
/// <seealso cref="Settings{AudioSettings}" />
class AudioSettings : public Settings<AudioSettings>
{
public:

    /// <summary>
    /// If checked, audio playback will be disabled in build game. Can be used if game uses custom audio playback engine.
    /// </summary>
    bool DisableAudio = false;

    /// <summary>
    /// The doppler effect factor. Scale for source and listener velocities. Default is 1.
    /// </summary>
    float DopplerFactor = 1.0f;

    /// <summary>
    /// True if mute all audio playback when game has no use focus.
    /// </summary>
    bool MuteOnFocusLoss = true;

public:

    // [Settings]
    void RestoreDefault() final override
    {
        DisableAudio = false;
        DopplerFactor = 1.0f;
        MuteOnFocusLoss = true;
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(DisableAudio);
        DESERIALIZE(DopplerFactor);
        DESERIALIZE(MuteOnFocusLoss);
    }
};
