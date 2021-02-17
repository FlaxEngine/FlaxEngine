// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Serialization/ISerializable.h"

/// <summary>
/// Base class for all global settings containers for the engine. Helps to apply, store and expose properties to engine/game.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API SettingsBase : public ISerializable
{
DECLARE_SCRIPTING_TYPE_MINIMAL(SettingsBase);
public:

    /// <summary>
    /// Applies the settings to the target system.
    /// </summary>
    virtual void Apply()
    {
    }

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override
    {
        // Not supported (Editor C# edits settings data)
    }
};

// Helper utility define for settings getter implementation code
#define IMPLEMENT_SETTINGS_GETTER(type, field) \
    type* type::Get() \
    { \
        static type DefaultInstance; \
        type* result = &DefaultInstance; \
        const auto gameSettings = GameSettings::Get(); \
        if (gameSettings) \
        { \
            const auto asset = Content::Load<JsonAsset>(gameSettings->field); \
            if (asset && asset->Instance && asset->InstanceType == type::TypeInitializer) \
            { \
                result = static_cast<type*>(asset->Instance); \
            } \
        } \
        return result; \
    }
