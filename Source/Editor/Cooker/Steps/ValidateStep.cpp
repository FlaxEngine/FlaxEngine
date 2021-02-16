// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ValidateStep.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/FileSystem.h"

bool ValidateStep::Perform(CookingData& data)
{
    data.StepProgress(TEXT("Performing validation"), 0);

    // Ensure output and cache directories exist
    if (!FileSystem::DirectoryExists(data.OutputPath))
    {
        if (FileSystem::CreateDirectory(data.OutputPath))
        {
            data.Error(TEXT("Failed to create build output directory."));
            return true;
        }
    }
    if (!FileSystem::DirectoryExists(data.CacheDirectory))
    {
        if (FileSystem::CreateDirectory(data.CacheDirectory))
        {
            data.Error(TEXT("Failed to create build cache directory."));
            return true;
        }
    }

#if OFFICIAL_BUILD
    // Validate that platform data is installed
    if (!FileSystem::DirectoryExists(data.GetGameBinariesPath()))
    {
        data.Error(TEXT("Missing platform data tools for the target platform. Use Flax Launcher and download the required package."));
        return true;
    }
#endif

    // Load game settings (may be modified via editor)
    if (GameSettings::Load())
    {
        data.Error(TEXT("Failed to load game settings."));
        return true;
    }
    data.AddRootAsset(Globals::ProjectContentFolder / TEXT("GameSettings.json"));

    // Validate game settings
    auto gameSettings = GameSettings::Get();
    {
        if (gameSettings->ProductName.IsEmpty())
        {
            data.Error(TEXT("Missing product name."));
            return true;
        }

        if (gameSettings->CompanyName.IsEmpty())
        {
            data.Error(TEXT("Missing company name."));
            return true;
        }

        // TODO: validate version

        AssetInfo info;
        if (!Content::GetAssetInfo(gameSettings->FirstScene, info))
        {
            data.Error(TEXT("Missing first scene. Set it in the game settings."));
            return true;
        }
    }

    // TODO: validate more game config

    // TODO: validate all input scenes?

    return false;
}
