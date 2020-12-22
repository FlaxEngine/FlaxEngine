// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GameSettings.h"
#include "Engine/Serialization/JsonTools.h"
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
#include "Engine/Engine/Time.h"

String GameSettings::ProductName;
String GameSettings::CompanyName;
String GameSettings::CopyrightNotice;
Guid GameSettings::Icon;
Guid GameSettings::FirstScene;
bool GameSettings::NoSplashScreen = false;
Guid GameSettings::SplashScreen;
Dictionary<String, Guid> GameSettings::CustomSettings;

Array<Settings*> Settings::Containers(32);

#if USE_EDITOR
extern void LoadPlatformSettingsEditor(ISerializable::DeserializeStream& data);
#endif

void TimeSettings::Apply()
{
    Time::UpdateFPS = UpdateFPS;
    Time::PhysicsFPS = PhysicsFPS;
    Time::DrawFPS = DrawFPS;
    Time::TimeScale = TimeScale;
}

class GameSettingsService : public EngineService
{
public:

    GameSettingsService()
        : EngineService(TEXT("GameSettings"), -70)
    {
        GameSettings::Icon = Guid::Empty;
        GameSettings::FirstScene = Guid::Empty;
        GameSettings::SplashScreen = Guid::Empty;
    }

    bool Init() override
    {
        return GameSettings::Load();
    }
};

GameSettingsService GameSettingsServiceInstance;

bool GameSettings::Load()
{
    CustomSettings.Clear();

#if USE_EDITOR
#define END_POINT(msg) LOG(Warning, msg " Using default values."); return false
#else
#define END_POINT(msg) LOG(Fatal, msg); return true
#endif

#define LOAD_SETTINGS(nodeName, settingsType) \
	{ \
		Guid id = JsonTools::GetGuid(data, nodeName); \
		if (id.IsValid()) \
		{ \
			AssetReference<JsonAsset> subAsset = Content::LoadAsync<JsonAsset>(id); \
			if (subAsset && !subAsset->WaitForLoaded()) \
			{ \
				settingsType::Instance()->Deserialize(*subAsset->Data, nullptr); \
                settingsType::Instance()->Apply(); \
			} \
			else \
			{ LOG(Warning, "Cannot load " nodeName " settings"); } \
		} \
		else \
		{ LOG(Warning, "Missing " nodeName " settings"); } \
	}

    // Load root game settings asset.
    // It may be missing in editor during dev but must be ready in the build game.
    const auto assetPath = Globals::ProjectContentFolder / TEXT("GameSettings.json");
    AssetReference<JsonAsset> asset = Content::LoadAsync<JsonAsset>(assetPath);
    if (asset == nullptr)
    {
        END_POINT("Missing game settings asset.");
    }
    if (asset->WaitForLoaded()
        || asset->DataTypeName != TEXT("FlaxEditor.Content.Settings.GameSettings")
        || asset->Data == nullptr)
    {
        END_POINT("Cannot load game settings asset.");
    }
    auto& data = *asset->Data;

    // Load settings
    ProductName = JsonTools::GetString(data, "ProductName");
    CompanyName = JsonTools::GetString(data, "CompanyName");
    CopyrightNotice = JsonTools::GetString(data, "CopyrightNotice");
    Icon = JsonTools::GetGuid(data, "Icon");
    FirstScene = JsonTools::GetGuid(data, "FirstScene");
    NoSplashScreen = JsonTools::GetBool(data, "NoSplashScreen", NoSplashScreen);
    SplashScreen = JsonTools::GetGuid(data, "SplashScreen");
    const auto customSettings = data.FindMember("CustomSettings");
    if (customSettings != data.MemberEnd())
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

    // Load child settings
    LOAD_SETTINGS("Time", TimeSettings);
    LOAD_SETTINGS("Physics", PhysicsSettings);
    LOAD_SETTINGS("LayersAndTags", LayersAndTagsSettings);
    LOAD_SETTINGS("Graphics", GraphicsSettings);
    LOAD_SETTINGS("GameCooking", BuildSettings);
    LOAD_SETTINGS("Input", InputSettings);
    LOAD_SETTINGS("Audio", AudioSettings);
    LOAD_SETTINGS("Navigation", NavigationSettings);

    // Load platform settings
#if PLATFORM_WINDOWS
    LOAD_SETTINGS("WindowsPlatform", WindowsPlatformSettings);
#endif
#if PLATFORM_UWP
    LOAD_SETTINGS("UWPPlatform", UWPPlatformSettings);
#endif
#if PLATFORM_LINUX
    LOAD_SETTINGS("LinuxPlatform", LinuxPlatformSettings);
#endif
#if PLATFORM_PS4
    LOAD_SETTINGS("PS4Platform", PS4PlatformSettings);
#endif
#if PLATFORM_XBOX_SCARLETT
    LOAD_SETTINGS("XboxScarlettPlatform", XboxScarlettPlatformSettings);
#endif
#if PLATFORM_ANDROID
    LOAD_SETTINGS("AndroidPlatform", AndroidPlatformSettings);
#endif
#if USE_EDITOR
    LoadPlatformSettingsEditor(data);
#endif

    return false;
#undef END_POINT
}
