// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "AudioQuality.h"

/// <summary>
/// Settings container for LOD options.
/// </summary>
API_STRUCT() struct AudioMixLODOptions : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(AudioMixLODOptions);

    /// <summary>
   /// Distance between mixer and audio source
   /// </summary>
    API_FIELD(Attributes = "EditorOrder(10)")
    float FaceOnDistance = 0.0f;

    /// <summary>
    /// Quality of audio.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(20)")
    AudioQuality QualityAudio;
};
