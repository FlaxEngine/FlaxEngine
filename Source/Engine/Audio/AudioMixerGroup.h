// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/ISerializable.h"
#include "AudioQuality.h"

/// <summary>
///  Settings container for a group of audio mixer settings. Defines the data audio mixer options.
/// </summary>
API_STRUCT() struct AudioMixerGroup : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(AudioMixerGroup);

    /// <summary>
    /// The name of the group.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(10)")
    String Name;

    /// <summary>
    ///  Volume of the pitch.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(20), Limit(0, 1)")
    float MixerVolume = 1.0f;

    /// <summary>
    /// Quality of audio.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(30)")
    AudioQuality QualityAudio;

};

