// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WINDOWS

#include "WindowsPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_SETTINGS_GETTER(WindowsPlatformSettings, WindowsPlatform);

const Char* WindowsPlatformTools::GetDisplayName() const
{
    return TEXT("Windows");
}

const Char* WindowsPlatformTools::GetName() const
{
    return TEXT("Windows");
}

PlatformType WindowsPlatformTools::GetPlatform() const
{
    return PlatformType::Windows;
}

ArchitectureType WindowsPlatformTools::GetArchitecture() const
{
    return _arch;
}

bool WindowsPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto platformSettings = WindowsPlatformSettings::Get();
    const auto gameSettings = GameSettings::Get();
    const auto& outputPath = data.CodeOutputPath;

    // Apply executable icon
    const String outputExePath = outputPath / TEXT("FlaxGame.exe");
    if (FileSystem::FileExists(outputExePath))
    {
        TextureData iconData;
        if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
        {
            if (EditorUtilities::UpdateExeIcon(outputExePath, iconData))
            {
                data.Error(TEXT("Failed to change output executable file icon."));
                return true;
            }
        }
      
#if !BUILD_DEBUG
        const String gameExePath = outputPath / gameSettings->ProductName / TEXT(".exe");
        if (gameExePath.Compare(outputExePath, StringSearchCase::IgnoreCase) != 0) {
            if (FileSystem::MoveFile(gameExePath, outputExePath, true))
            {
                data.Error(TEXT("Failed to rename output executable file."));
                return true;
            }
        }
#endif

    }
    return false;
}

#endif
