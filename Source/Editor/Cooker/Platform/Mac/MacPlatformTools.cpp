// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_MAC

#include "MacPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Mac/MacPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_SETTINGS_GETTER(MacPlatformSettings, MacPlatform);

MacPlatformTools::MacPlatformTools(ArchitectureType arch)
    : _arch(arch)
{
}

const Char* MacPlatformTools::GetDisplayName() const
{
    return TEXT("Mac");
}

const Char* MacPlatformTools::GetName() const
{
    return TEXT("Mac");
}

PlatformType MacPlatformTools::GetPlatform() const
{
    return PlatformType::Mac;
}

ArchitectureType MacPlatformTools::GetArchitecture() const
{
    return _arch;
}

bool MacPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = MacPlatformSettings::Get();
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
#if PLATFORM_MAC
    system(*StringAnsi(String::Format(TEXT("chmod +x \"{0}\""), gameExePath)));
#endif

    return false;
}

#endif
