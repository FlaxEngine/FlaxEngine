// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/ISerializable.h"
#if USE_EDITOR
#include "Engine/Core/Collections/Dictionary.h"
#endif

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
    /// 
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(15), Limit(0, 1)")
    float PitchMixer;

};

