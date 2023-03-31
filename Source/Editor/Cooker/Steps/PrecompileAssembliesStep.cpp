// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "PrecompileAssembliesStep.h"
#include "Engine/Platform/FileSystem.h"
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

    // Redirect C# assemblies to intermediate cooking directory (processed by ILC)
    data.ManagedCodeOutputPath = data.CacheDirectory / TEXT("AOTAssemblies");
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

    // Run AOT by Flax.Build
    const Char *platform, *architecture, *configuration = ::ToString(data.Configuration);
    data.GetBuildPlatformName(platform, architecture);
    const String logFile = data.CacheDirectory / TEXT("AotLog.txt");
    const Char* aotModeName = TEXT("");
    switch (aotMode)
    {
    case DotNetAOTModes::ILC:
        aotModeName = TEXT("ILC");
        break;
    case DotNetAOTModes::MonoAOTDynamic:
        aotModeName = TEXT("MonoAOTDynamic");
        break;
    case DotNetAOTModes::MonoAOTStatic:
        aotModeName = TEXT("MonoAOTStatic");
        break;
    }
    auto args = String::Format(
        TEXT("-log -logfile=\"{}\" -runDotNetAOT -mutex -platform={} -arch={} -configuration={} -aotMode={} -binaries=\"{}\" -intermediate=\"{}\""),
        logFile, platform, architecture, configuration, aotModeName, data.DataOutputPath, data.ManagedCodeOutputPath);
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

    // Useful references about AOT:
    // https://github.com/dotnet/runtime/blob/main/src/coreclr/nativeaot/docs/README.md
    // https://github.com/dotnet/runtime/blob/main/docs/workflow/building/coreclr/nativeaot.md
    // https://github.com/dotnet/samples/tree/main/core/nativeaot/NativeLibrary
    // http://www.mono-project.com/docs/advanced/runtime/docs/aot/
    // http://www.mono-project.com/docs/advanced/aot/

    // Setup
    // TODO: remove old AotConfig, OnConfigureAOT, OnPerformAOT and OnPostProcessAOT
    PlatformTools::AotConfig config(data);
    data.Tools->OnConfigureAOT(data, config);

    // Prepare output directory
    config.AotCachePath = data.DataOutputPath / TEXT("Mono/lib/mono/aot-cache");
    switch (data.Tools->GetArchitecture())
    {
    case ArchitectureType::x86:
        config.AotCachePath /= TEXT("x86");
        break;
    case ArchitectureType::x64:
        config.AotCachePath /= TEXT("amd64");
        break;
    default:
        data.Error(TEXT("Not supported AOT architecture"));
        return true;
    }
    if (!FileSystem::DirectoryExists(config.AotCachePath))
    {
        if (FileSystem::CreateDirectory(config.AotCachePath))
        {
            data.Error(TEXT("Failed to setup AOT output directory."));
            return true;
        }
    }

    // Collect assemblies for AOT
    // TODO: don't perform AOT on all assemblies but only ones used by the game and engine assemblies
    for (auto& dir : config.AssembliesSearchDirs)
        FileSystem::DirectoryGetFiles(config.Assemblies, dir, TEXT("*.dll"), DirectorySearchOption::TopDirectoryOnly);
    for (auto& binaryModule : data.BinaryModules)
        if (binaryModule.ManagedPath.HasChars())
            config.Assemblies.Add(data.ManagedCodeOutputPath / binaryModule.ManagedPath);
    // TODO: move AOT to Flax.Build and perform it on all C# assemblies used in target build
    config.Assemblies.Add(data.ManagedCodeOutputPath / TEXT("Newtonsoft.Json.dll"));

    // Perform AOT for the assemblies
    for (int32 i = 0; i < config.Assemblies.Count(); i++)
    {
        BUILD_STEP_CANCEL_CHECK;

        if (data.Tools->OnPerformAOT(data, config, config.Assemblies[i]))
            return true;

        data.StepProgress(infoMsg, static_cast<float>(i) / config.Assemblies.Count());
    }

    BUILD_STEP_CANCEL_CHECK;

    if (data.Tools->OnPostProcessAOT(data, config))
        return true;

    // TODO: maybe remove GAC/assemblies? aot-cache could be only used in the build game

    return false;
}
