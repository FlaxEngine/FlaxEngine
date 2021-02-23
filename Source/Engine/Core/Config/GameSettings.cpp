// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GameSettings.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Core/Log.h"
#include "LayersTagsSettings.h"
#include "TimeSettings.h"
#include "PlatformSettings.h"
#include "GraphicsSettings.h"
#include "BuildSettings.h"
#include "Engine/Input/InputSettings.h"
#include "Engine/Audio/AudioSettings.h"
#include "Engine/Navigation/NavigationSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Globals.h"

class GameSettingsService : public EngineService
{
public:

    GameSettingsService()
        : EngineService(TEXT("GameSettings"), -70)
    {
    }

    bool Init() override
    {
        return GameSettings::Load();
    }
};

IMPLEMENT_SETTINGS_GETTER(BuildSettings, GameCooking);
IMPLEMENT_SETTINGS_GETTER(GraphicsSettings, Graphics);
IMPLEMENT_SETTINGS_GETTER(LayersAndTagsSettings, LayersAndTags);
IMPLEMENT_SETTINGS_GETTER(TimeSettings, Time);
IMPLEMENT_SETTINGS_GETTER(AudioSettings, Audio);
IMPLEMENT_SETTINGS_GETTER(PhysicsSettings, Physics);
IMPLEMENT_SETTINGS_GETTER(InputSettings, Input);

#if !USE_EDITOR
#if PLATFORM_WINDOWS
IMPLEMENT_SETTINGS_GETTER(WindowsPlatformSettings, WindowsPlatform);
#elif PLATFORM_UWP || PLATFORM_XBOX_ONE
IMPLEMENT_SETTINGS_GETTER(UWPPlatformSettings, UWPPlatform);
#elif PLATFORM_LINUX
IMPLEMENT_SETTINGS_GETTER(LinuxPlatformSettings, LinuxPlatform);
#elif PLATFORM_PS4
IMPLEMENT_SETTINGS_GETTER(PS4PlatformSettings, PS4Platform);
#elif PLATFORM_XBOX_SCARLETT
IMPLEMENT_SETTINGS_GETTER(XboxScarlettPlatformSettings, XboxScarlettPlatform);
#elif PLATFORM_ANDROID
IMPLEMENT_SETTINGS_GETTER(AndroidPlatformSettings, AndroidPlatform);
#else
#error Unknown platform
#endif
#endif

GameSettingsService GameSettingsServiceInstance;
AssetReference<JsonAsset> GameSettingsAsset;

GameSettings* GameSettings::Get()
{
    if (!GameSettingsAsset)
    {
        // Load root game settings asset.
        // It may be missing in editor during dev but must be ready in the build game.
        const auto assetPath = Globals::ProjectContentFolder / TEXT("GameSettings.json");
        GameSettingsAsset = Content::LoadAsync<JsonAsset>(assetPath);
        if (GameSettingsAsset == nullptr)
        {
            LOG(Error, "Missing game settings asset.");
            return nullptr;
        }
        if (GameSettingsAsset->WaitForLoaded())
        {
            return nullptr;
        }
        if (GameSettingsAsset->InstanceType != GameSettings::TypeInitializer)
        {
            LOG(Error, "Invalid game settings asset data type.");
            return nullptr;
        }
    }
    auto asset = GameSettingsAsset.Get();
    if (asset && asset->WaitForLoaded())
        asset = nullptr;
    return asset ? (GameSettings*)asset->Instance : nullptr;
}

bool GameSettings::Load()
{
    // Load main settings asset
    auto settings = Get();
    if (!settings)
    {
        return true;
    }

    // Preload all settings assets
#define PRELOAD_SETTINGS(type) \
    { \
        if (settings->type) \
        { \
            Content::LoadAsync<JsonAsset>(settings->type); \
        } \
        else \
        { \
            LOG(Warning, "Missing {0} settings", TEXT(#type)); \
        } \
    }
    PRELOAD_SETTINGS(Time);
    PRELOAD_SETTINGS(Audio);
    PRELOAD_SETTINGS(LayersAndTags);
    PRELOAD_SETTINGS(Physics);
    PRELOAD_SETTINGS(Input);
    PRELOAD_SETTINGS(Graphics);
    PRELOAD_SETTINGS(Navigation);
    PRELOAD_SETTINGS(GameCooking);
#undef PRELOAD_SETTINGS

    // Apply the game settings to the engine
    settings->Apply();

    return false;
}

void GameSettings::Apply()
{
    // TODO: impl this
#define APPLY_SETTINGS(type) \
    { \
        type* obj = type::Get(); \
        if (obj) \
        { \
            obj->Apply(); \
        } \
        else \
        { \
            LOG(Warning, "Missing {0} settings", TEXT(#type)); \
        } \
    }
    APPLY_SETTINGS(TimeSettings);
    APPLY_SETTINGS(AudioSettings);
    APPLY_SETTINGS(LayersAndTagsSettings);
    APPLY_SETTINGS(PhysicsSettings);
    APPLY_SETTINGS(InputSettings);
    APPLY_SETTINGS(GraphicsSettings);
    APPLY_SETTINGS(NavigationSettings);
    APPLY_SETTINGS(BuildSettings);
    APPLY_SETTINGS(PlatformSettings);
#undef APPLY_SETTINGS
}

void GameSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Load properties
    ProductName = JsonTools::GetString(stream, "ProductName");
    CompanyName = JsonTools::GetString(stream, "CompanyName");
    CopyrightNotice = JsonTools::GetString(stream, "CopyrightNotice");
    Icon = JsonTools::GetGuid(stream, "Icon");
    FirstScene = JsonTools::GetGuid(stream, "FirstScene");
    NoSplashScreen = JsonTools::GetBool(stream, "NoSplashScreen", NoSplashScreen);
    SplashScreen = JsonTools::GetGuid(stream, "SplashScreen");
    CustomSettings.Clear();
    const auto customSettings = stream.FindMember("CustomSettings");
    if (customSettings != stream.MemberEnd())
    {
        auto& items = customSettings->value;
        for (auto it = items.MemberBegin(); it != items.MemberEnd(); ++it)
        {
            if (it->value.IsString() && it->value.GetStringLength() == 32)
            {
                String key = it->name.GetText();
                const Guid value = JsonTools::GetGuid(it->value);
                CustomSettings[key] = value;
            }
        }
    }

    // Settings containers
    DESERIALIZE(Time);
    DESERIALIZE(Audio);
    DESERIALIZE(LayersAndTags);
    DESERIALIZE(Physics);
    DESERIALIZE(Input);
    DESERIALIZE(Graphics);
    DESERIALIZE(Navigation);
    DESERIALIZE(GameCooking);

    // Per-platform settings containers
    DESERIALIZE(WindowsPlatform);
    DESERIALIZE(UWPPlatform);
    DESERIALIZE(LinuxPlatform);
    DESERIALIZE(PS4Platform);
    DESERIALIZE(XboxScarlettPlatform);
    DESERIALIZE(AndroidPlatform);
}

void LayersAndTagsSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    const auto tags = stream.FindMember("Tags");
    if (tags != stream.MemberEnd() && tags->value.IsArray())
    {
        auto& tagsArray = tags->value;
        Tags.Clear();
        Tags.EnsureCapacity(tagsArray.Size());
        for (uint32 i = 0; i < tagsArray.Size(); i++)
        {
            auto& v = tagsArray[i];
            if (v.IsString())
                Tags.Add(v.GetText());
        }
    }

    const auto layers = stream.FindMember("Layers");
    if (layers != stream.MemberEnd() && layers->value.IsArray())
    {
        auto& layersArray = layers->value;
        for (uint32 i = 0; i < layersArray.Size() && i < 32; i++)
        {
            auto& v = layersArray[i];
            if (v.IsString())
                Layers[i] = v.GetText();
            else
                Layers[i].Clear();
        }
        for (uint32 i = layersArray.Size(); i < 32; i++)
        {
            Layers[i].Clear();
        }
    }
}
