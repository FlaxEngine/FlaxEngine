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

bool WindowsPlatformTools::UseAOT() const
{
    return true;
}

void WindowsPlatformTools::OnConfigureAOT(CookingData& data, AotConfig& config)
{
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const bool useInterpreter = true; // TODO: support using Full AOT instead of interpreter
    const bool enableDebug = data.Configuration != BuildConfiguration::Release;
    const Char* aotMode = useInterpreter ? TEXT("full,interp") : TEXT("full");
    const Char* debugMode = enableDebug ? TEXT("soft-debug") : TEXT("nodebug");
    config.AotCompilerArgs = String::Format(TEXT("--aot={0},verbose,stats,print-skipped,{1} -O=all"),
        aotMode,
        debugMode);
    if (enableDebug)
        config.AotCompilerArgs = TEXT("--debug ") + config.AotCompilerArgs;
    config.AotCompilerPath = platformDataPath / TEXT("Tools/mono.exe");
}

bool WindowsPlatformTools::OnPerformAOT(CookingData& data, AotConfig& config, const String& assemblyPath)
{
    // Skip .dll.dll which could be a false result from the previous AOT which could fail
    if (assemblyPath.EndsWith(TEXT(".dll.dll")))
    {
        LOG(Warning, "Skip AOT for file '{0}' as it can be a result from the previous task", assemblyPath);
        return false;
    }

    // Check if skip this assembly (could be already processed)
    const String filename = StringUtils::GetFileName(assemblyPath);
    const String outputPath = config.AotCachePath / filename + TEXT(".dll");
    if (FileSystem::FileExists(outputPath) && FileSystem::GetFileLastEditTime(assemblyPath) < FileSystem::GetFileLastEditTime(outputPath))
        return false;
    LOG(Info, "Calling AOT tool for \"{0}\"", assemblyPath);

    // Cleanup temporary results (fromm the previous AT that fail or sth)
    const String resultPath = assemblyPath + TEXT(".dll");
    const String resultPathExp = resultPath + TEXT(".exp");
    const String resultPathLib = resultPath + TEXT(".lib");
    const String resultPathPdb = resultPath + TEXT(".pdb");
    if (FileSystem::FileExists(resultPath))
        FileSystem::DeleteFile(resultPath);
    if (FileSystem::FileExists(resultPathExp))
        FileSystem::DeleteFile(resultPathExp);
    if (FileSystem::FileExists(resultPathLib))
        FileSystem::DeleteFile(resultPathLib);
    if (FileSystem::FileExists(resultPathPdb))
        FileSystem::DeleteFile(resultPathPdb);

    // Call tool
    String workingDir = StringUtils::GetDirectoryName(config.AotCompilerPath);
    String command = String::Format(TEXT("\"{0}\" {1} \"{2}\""), config.AotCompilerPath, config.AotCompilerArgs, assemblyPath);
    const int32 result = Platform::RunProcess(command, workingDir, config.EnvVars);
    if (result != 0)
    {
        data.Error(TEXT("AOT tool execution failed with result code {1} for assembly \"{0}\". See log for more info."), assemblyPath, result);
        return true;
    }

    // Copy result
    if (FileSystem::CopyFile(outputPath, resultPath))
    {
        data.Error(TEXT("Failed to copy the AOT tool result file. It can be missing."));
        return true;
    }

    // Copy pdb file if exists
    if (data.Configuration != BuildConfiguration::Release && FileSystem::FileExists(resultPathPdb))
    {
        FileSystem::CopyFile(config.AotCachePath / StringUtils::GetFileName(resultPathPdb), resultPathPdb);
    }

    // Clean intermediate results
    if (FileSystem::DeleteFile(resultPath)
        || (FileSystem::FileExists(resultPathExp) && FileSystem::DeleteFile(resultPathExp))
        || (FileSystem::FileExists(resultPathLib) && FileSystem::DeleteFile(resultPathLib))
        || (FileSystem::FileExists(resultPathPdb) && FileSystem::DeleteFile(resultPathPdb))
        )
    {
        LOG(Warning, "Failed to remove the AOT tool result file(s).");
    }

    return false;
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
