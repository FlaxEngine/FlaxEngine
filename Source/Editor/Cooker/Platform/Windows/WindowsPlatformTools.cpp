// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WINDOWS

#include "WindowsPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_ENGINE_SETTINGS_GETTER(WindowsPlatformSettings, WindowsPlatform);

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

bool WindowsPlatformTools::UseSystemDotnet() const
{
    return true;
}

bool WindowsPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto platformSettings = WindowsPlatformSettings::Get();

    // Apply executable icon
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
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

void WindowsPlatformTools::OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir)
{
    // Pick the first executable file
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
    if (files.HasItems())
    {
        executableFile = files[0];
    }
}

#endif
