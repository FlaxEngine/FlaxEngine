// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"

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
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override
    {
    }
};

// Helper utility define for Engine settings getter implementation code
#define IMPLEMENT_ENGINE_SETTINGS_GETTER(type, field) \
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

// [Deprecated on 20.01.2022, expires on 20.01.2024]
#define IMPLEMENT_SETTINGS_GETTER(type, field) IMPLEMENT_ENGINE_SETTINGS_GETTER(type, field)

// Helper utility define for Game settings getter implementation code
#define IMPLEMENT_GAME_SETTINGS_GETTER(type, name) \
    type* type::Get() \
    { \
        static type DefaultInstance; \
        type* result = &DefaultInstance; \
        const auto gameSettings = GameSettings::Get(); \
        if (gameSettings) \
        { \
            Guid assetId = Guid::Empty; \
            gameSettings->CustomSettings.TryGet(TEXT(name), assetId); \
            const auto asset = Content::Load<JsonAsset>(assetId); \
            if (asset) \
            { \
                if (auto* instance = asset->GetInstance<type>()) \
                    result = instance; \
            } \
        } \
        return result; \
    }

#define DECLARE_SETTINGS_GETTER(type) static type* Get()
