// Copyright (c) Wojciech Figat. All rights reserved.

#include "PrecompileAssembliesStep.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Engine/Globals.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Editor/Utilities/EditorUtilities.h"

void PrecompileAssembliesStep::OnBuildStarted(CookingData& data)
{
    const DotNetAOTModes aotMode = data.Tools->UseAOT();
    if (aotMode == DotNetAOTModes::None)
        return;
    const auto& buildSettings = *BuildSettings::Get();

    // Redirect C# assemblies to intermediate cooking directory (processed by ILC)
    data.ManagedCodeOutputPath = data.CacheDirectory / TEXT("AOTAssemblies");

    // Reset any AOT cache from previous run if the AOT mode has changed (eg. Mono AOT -> ILC on Desktop)
    const String aotModeCacheFilePath = data.ManagedCodeOutputPath / TEXT("AOTMode.txt");
    String aotModeCacheValue = String::Format(TEXT("{};{};{};{}"),
                                              (int32)aotMode,
                                              (int32)data.Configuration,
                                              (int32)buildSettings.SkipUnusedDotnetLibsPackaging,
                                              FileSystem::GetFileLastEditTime(ScriptsBuilder::GetBuildToolPath()).Ticks);
    for (const String& define : data.CustomDefines)
        aotModeCacheValue += define;
    if (FileSystem::DirectoryExists(data.ManagedCodeOutputPath))
    {
        String cachedData;
        File::ReadAllText(aotModeCacheFilePath, cachedData);
        if (cachedData != aotModeCacheValue)
        {
            LOG(Info, "AOT cache invalidation");
            FileSystem::DeleteDirectory(data.ManagedCodeOutputPath); // Remove AOT cache
            FileSystem::DeleteDirectory(data.DataOutputPath / TEXT("Dotnet")); // Remove deployed Dotnet libs (be sure to remove any leftovers from previous build)
        }
    }
    if (!FileSystem::DirectoryExists(data.ManagedCodeOutputPath))
    {
        FileSystem::CreateDirectory(data.ManagedCodeOutputPath);
        File::WriteAllText(aotModeCacheFilePath, aotModeCacheValue, Encoding::ANSI);
    }
}

bool PrecompileAssembliesStep::Perform(CookingData& data)
{
    const DotNetAOTModes aotMode = data.Tools->UseAOT();
    if (aotMode == DotNetAOTModes::None)
        return false;
    const auto& buildSettings = *BuildSettings::Get();
    if (buildSettings.SkipDotnetPackaging && data.Tools->UseSystemDotnet())
        return false;
    LOG(Info, "Using AOT...");
    const String infoMsg = TEXT("Running AOT");
    data.StepProgress(infoMsg, 0);

    // Override Newtonsoft.Json with AOT-version (one that doesn't use System.Reflection.Emit)
    EditorUtilities::CopyFileIfNewer(data.ManagedCodeOutputPath / TEXT("Newtonsoft.Json.dll"), Globals::StartupFolder / TEXT("Source/Platforms/DotNet/AOT/Newtonsoft.Json.dll"));
    FileSystem::DeleteFile(data.ManagedCodeOutputPath / TEXT("Newtonsoft.Json.xml"));
    FileSystem::DeleteFile(data.ManagedCodeOutputPath / TEXT("Newtonsoft.Json.pdb"));

    // Run AOT by Flax.Build (see DotNetAOT)
    const Char *platform, *architecture, *configuration = ::ToString(data.Configuration);
    data.GetBuildPlatformName(platform, architecture);
    const String logFile = data.CacheDirectory / TEXT("AOTLog.txt");
    String args = String::Format(
        TEXT("-log -logfile=\"{}\" -runDotNetAOT -mutex -platform={} -arch={} -configuration={} -aotMode={} -binaries=\"{}\" -intermediate=\"{}\" {}"),
        logFile, platform, architecture, configuration, ToString(aotMode), data.DataOutputPath, data.ManagedCodeOutputPath, GAME_BUILD_DOTNET_VER);
    if (!buildSettings.SkipUnusedDotnetLibsPackaging)
        args += TEXT(" -skipUnusedDotnetLibs=false"); // Run AOT on whole class library (not just used libs)
    for (const String& define : data.CustomDefines)
    {
        args += TEXT(" -D");
        args += define;
    }
    if (ScriptsBuilder::RunBuildTool(args))
    {
        data.Error(TEXT("Failed to precompile game scripts."));
        return true;
    }

    return false;
}
