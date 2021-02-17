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
    const auto& outputPath = data.OutputPath;

    // Apply executable icon
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, outputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
    if (files.HasItems())
    {
        TextureData iconData;
        if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
        {
            if (EditorUtilities::UpdateExeIcon(files[0], iconData))
            {
                data.Error(TEXT("Failed to change output executable file icon."));
                return true;
            }
        }
    }

    return false;
}

#endif
