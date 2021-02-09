// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Singleton.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Serialization/ISerializable.h"

/// <summary>
/// Base class for all global settings containers for the engine. Helps to apply, store and expose properties to c#.
/// </summary>
class FLAXENGINE_API SettingsBase
{
public:

    /// <summary>
    /// The settings containers.
    /// </summary>
    static Array<SettingsBase*> Containers;

    /// <summary>
    /// Restores the default settings for all the registered containers.
    /// </summary>
    static void RestoreDefaultAll()
    {
        for (int32 i = 0; i < Containers.Count(); i++)
            Containers[i]->RestoreDefault();
    }

private:

    // Disable copy/move
    SettingsBase(const SettingsBase&) = delete;
    SettingsBase& operator=(const SettingsBase&) = delete;

protected:

    SettingsBase()
    {
        Containers.Add(this);
    }

public:

    virtual ~SettingsBase() = default;

public:

    typedef ISerializable::DeserializeStream DeserializeStream;

    /// <summary>
    /// Applies the settings to the target services.
    /// </summary>
    virtual void Apply()
    {
    }

    /// <summary>
    /// Restores the default settings.
    /// </summary>
    virtual void RestoreDefault() = 0;

    /// <summary>
    /// Deserializes the settings container.
    /// </summary>
    /// <param name="stream">The input data stream.</param>
    /// <param name="modifier">The deserialization modifier object. Always valid.</param>
    virtual void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) = 0;
};

/// <summary>
/// Base class for all global settings containers for the engine. Helps to apply, store and expose properties to c#.
/// </summary>
template<class T>
class Settings : public SettingsBase, public Singleton<T>
{
protected:

    Settings()
    {
    }
};
