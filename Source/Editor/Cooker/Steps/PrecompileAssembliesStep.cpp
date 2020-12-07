// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "PrecompileAssembliesStep.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Platform/FileSystem.h"
#include "Editor/Cooker/PlatformTools.h"

bool PrecompileAssembliesStep::Perform(CookingData& data)
{
    // Skip for some platforms
    if (!data.Tools->UseAOT())
        return false;
    LOG(Info, "Using AOT...");

    // Useful references about AOT:
    // http://www.mono-project.com/docs/advanced/runtime/docs/aot/
    // http://www.mono-project.com/docs/advanced/aot/

    const String infoMsg = TEXT("Running AOT");
    data.StepProgress(infoMsg, 0);

    // Setup
    PlatformTools::AotConfig config(data);
    data.Tools->OnConfigureAOT(data, config);

    // Prepare output directory
    config.AotCachePath = data.OutputPath / TEXT("Mono/lib/mono/aot-cache");
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
            config.Assemblies.Add(data.OutputPath / binaryModule.ManagedPath);
    // TODO: move AOT to Flax.Build and perform it on all C# assemblies used in target build
    config.Assemblies.Add(data.OutputPath / TEXT("Newtonsoft.Json.dll"));

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
