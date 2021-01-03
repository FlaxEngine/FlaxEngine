// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_SHADER_COMPILER

#include "ShaderCompilationContext.h"
#include "Engine/Core/Log.h"
#include "Parser/ShaderMeta.h"
#include "Engine/Graphics/Config.h"
#include "Config.h"

void ShaderCompilationContext::OnError(const char* message)
{
    LOG(Error, "Failed to compile '{0}'. {1}", Options->TargetName, String(message));
}

void ShaderCompilationContext::OnCollectDebugInfo(ShaderFunctionMeta& meta, int32 permutationIndex, const char* data, const int32 dataLength)
{
#ifdef GPU_USE_SHADERS_DEBUG_LAYER

    // Cache data
    meta.Permutations[permutationIndex].DebugData.Set(data, dataLength);

#endif
}

ShaderCompilationContext::ShaderCompilationContext(const ShaderCompilationOptions* options, ShaderMeta* meta)
    : Options(options)
    , Meta(meta)
    , Output(options->Output)
{
    // Convert target name to ANSI text (with limited length)
    const int32 ansiNameLen = Math::Min<int32>(ARRAY_COUNT(TargetNameAnsi) - 1, options->TargetName.Length());
    StringUtils::ConvertUTF162ANSI(*options->TargetName, TargetNameAnsi, ansiNameLen);
    TargetNameAnsi[ansiNameLen] = 0;
}

#endif
