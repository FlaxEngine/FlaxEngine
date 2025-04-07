// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderCompilationContext.h"
#include "Engine/Graphics/Config.h"

#if GPU_USE_SHADERS_DEBUG_LAYER && COMPILE_WITH_SHADER_COMPILER

#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Parser/ShaderMeta.h"

/// <summary>
/// Tool class used to export debug information about shaders
/// </summary>
class ShaderDebugDataExporter
{
public:

    /// <summary>
    /// Exports compilation results info
    /// </summary>
    /// <param name="context">Compilation context</param>
    /// <returns>True if failed, otherwise false</returns>
    static bool Export(ShaderCompilationContext* context)
    {
        PROFILE_CPU();
#if USE_EDITOR
        static String ShadersDebugInfoFolder = Globals::ProjectCacheFolder / TEXT("Shaders/Debug");
#else
        static String ShadersDebugInfoFolder = Globals::ProductLocalFolder / TEXT("Shaders/Debug");
#endif

        // Create output folder
        if (!FileSystem::DirectoryExists(ShadersDebugInfoFolder))
        {
            if (FileSystem::CreateDirectory(ShadersDebugInfoFolder))
                return true;
        }

        // Prepare
        auto options = context->Options;
        String outputFilePath = ShadersDebugInfoFolder / String::Format(TEXT("ShaderDebug_{0}_{1}.txt"), options->TargetName, options->TargetID);
        auto shadersCount = context->Meta->GetShadersCount();
        StringBuilder info(1024 * shadersCount);
        Array<const ShaderFunctionMeta*> functions(shadersCount);
        context->Meta->GetShaders(functions);

        // Generate output info
        info.AppendFormat(TEXT("Target shader: {0} : {1}\nProfile: {2}\nCache size: {3} bytes\n"),
                          options->TargetName,
                          options->TargetID,
                          ::ToString(options->Profile),
                          options->Output->GetPosition());
        for (int32 i = 0; i < functions.Count(); i++)
        {
            const auto f = functions[i];
            auto stageName = ::ToString(f->GetStage());
            for (int32 permutationIndex = 0; permutationIndex < f->Permutations.Count(); permutationIndex++)
            {
                info.Append(TEXT("\n*********************************************************************\n"));
                info.AppendFormat(TEXT("{0} Shader: {1}, Permutation: {2}\n"), stageName, String(f->Name), permutationIndex + 1);
                info.Append(f->Permutations[permutationIndex].DebugData.Get());
            }
        }

#if PLATFORM_WINDOWS

        // Change line endings on Windows platform
        StringBuilder winNewLineFix = info;
        WindowsFileSystem::ConvertLineEndingsToDos(StringView(*winNewLineFix, winNewLineFix.Length()), info.GetCharArray());

#endif

        // Write to file
        return File::WriteAllText(outputFilePath, info, Encoding::Unicode);
    }
};

#endif
