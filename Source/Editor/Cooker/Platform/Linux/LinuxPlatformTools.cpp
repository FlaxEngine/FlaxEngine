// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_LINUX

#include "LinuxPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Linux/LinuxPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_ENGINE_SETTINGS_GETTER(LinuxPlatformSettings, LinuxPlatform);

const Char* LinuxPlatformTools::GetDisplayName() const
{
    return TEXT("Linux");
}

const Char* LinuxPlatformTools::GetName() const
{
    return TEXT("Linux");
}

PlatformType LinuxPlatformTools::GetPlatform() const
{
    return PlatformType::Linux;
}

ArchitectureType LinuxPlatformTools::GetArchitecture() const
{
    return ArchitectureType::x64;
}

bool LinuxPlatformTools::UseSystemDotnet() const
{
    return true;
}

bool LinuxPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = LinuxPlatformSettings::Get();
    const auto outputPath = data.DataOutputPath;

    // Copy binaries
    {
        if (!FileSystem::DirectoryExists(outputPath))
            FileSystem::CreateDirectory(outputPath);
        const auto binPath = data.GetGameBinariesPath();

        // Gather files to deploy
        Array<String> files;
        files.Add(binPath / TEXT("FlaxGame"));
        FileSystem::DirectoryGetFiles(files, binPath, TEXT("*.a"), DirectorySearchOption::TopDirectoryOnly);

        // Copy data
        for (int32 i = 0; i < files.Count(); i++)
        {
            if (FileSystem::CopyFile(outputPath / StringUtils::GetFileName(files[i]), files[i]))
            {
                data.Error(TEXT("Failed to setup output directory."));
                return true;
            }
        }
    }

    // Apply game executable file name
#if !BUILD_DEBUG
	const String outputExePath = outputPath / TEXT("FlaxGame");
	const String gameExePath = outputPath / gameSettings->ProductName;
	if (FileSystem::FileExists(outputExePath) && gameExePath.Compare(outputExePath, StringSearchCase::IgnoreCase) != 0)
	{
		if (FileSystem::MoveFile(gameExePath, outputExePath, true))
		{
			data.Error(TEXT("Failed to rename output executable file."));
			return true;
		}
	}
#else
    // Don't change application name on a DEBUG build (for build game debugging)
    const String gameExePath = outputPath / TEXT("FlaxGame");
#endif

    // Ensure the output binary can be executed
#if PLATFORM_LINUX
    system(*StringAnsi(String::Format(TEXT("chmod +x \"{0}\""), gameExePath)));
#endif

    // Apply game icon
    TextureData iconData;
    if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
    {
        const String iconPath = outputPath / TEXT("Content/icon.png");
        if (TextureTool::ExportTexture(iconPath, iconData))
        {
            data.Error(TEXT("Failed to export game icon."));
            return true;
        }
    }

    return false;
}

void LinuxPlatformTools::OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir)
{
    // Pick the first executable file
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
    for (auto& file : files)
    {
        if (FileSystem::GetExtension(file).IsEmpty())
        {
            executableFile = file;
            break;
        }
    }
}

#endif
