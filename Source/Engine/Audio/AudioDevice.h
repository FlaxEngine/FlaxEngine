// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Represents a single audio device.
/// </summary>
API_CLASS(NoSpawn) class AudioDevice : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(AudioDevice);

    explicit AudioDevice()
        : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    {
    }

    AudioDevice(const AudioDevice& other)
        : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    {
        Name = other.Name;
        InternalName = other.InternalName;
    }

    AudioDevice& operator=(const AudioDevice& other)
    {
        Name = other.Name;
        InternalName = other.InternalName;
        return *this;
    }

public:

    /// <summary>
    /// The device name.
    /// </summary>
    API_FIELD(ReadOnly) String Name;

    /// <summary>
    /// The internal device name used by the audio backend.
    /// </summary>
    StringAnsi InternalName;
};
