// Copyright (c) Wojciech Figat. All rights reserved.

#include "PostProcessStep.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Engine/Platform/FileSystem.h"

bool PostProcessStep::Perform(CookingData& data)
{
    // Print .NET stats
    const DotNetAOTModes aotMode = data.Tools->UseAOT();
    uint64 outputSize = FileSystem::GetDirectorySize(data.DataOutputPath / TEXT("Dotnet"));
    if (aotMode == DotNetAOTModes::None)
    {
        for (auto& binaryModule : data.BinaryModules)
            outputSize += FileSystem::GetFileSize(data.DataOutputPath / binaryModule.ManagedPath);
    }
    LOG(Info, "Output .NET files size: {0} MB", (uint32)(outputSize / (1024ull * 1024)));

    GameCooker::PostProcessFiles();
    return data.Tools->OnPostProcess(data);
}
