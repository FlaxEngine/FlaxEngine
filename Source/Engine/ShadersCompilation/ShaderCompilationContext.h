// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_SHADER_COMPILER

#include "Config.h"
#include "Engine/Core/Collections/HashSet.h"

class ShaderMeta;
class ShaderFunctionMeta;
class MemoryWriteStream;

/// <summary>
/// Shader compilation context container
/// </summary>
class ShaderCompilationContext
{
public:

    /// <summary>
    /// The compilation options.
    /// </summary>
    const ShaderCompilationOptions* Options;

    /// <summary>
    /// The shader metadata container.
    /// </summary>
    ShaderMeta* Meta;

public:

    /// <summary>
    /// Output stream to write compiled shader cache to.
    /// </summary>
    MemoryWriteStream* Output;

    /// <summary>
    /// All source files included by this file (absolute paths). Generated during shader compilation.
    /// </summary>
    HashSet<String> Includes;

public:

    /// <summary>
    /// Name of the target object (in ASCII)
    /// </summary>
    char TargetNameAnsi[64];

public:

    /// <summary>
    /// Event called on compilation error
    /// </summary>
    /// <param name="message">Error message</param>
    void OnError(const char* message);

    /// <summary>
    /// Event called on compilation debug data collecting
    /// </summary>
    /// <param name="meta">Target function meta</param>
    /// <param name="permutationIndex">Permutation index</param>
    /// <param name="data">Data pointer</param>
    /// <param name="dataLength">Data size in bytes</param>
    void OnCollectDebugInfo(ShaderFunctionMeta& meta, int32 permutationIndex, const char* data, const int32 dataLength);

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="options">Options</param>
    /// <param name="meta">Metadata</param>
    ShaderCompilationContext(const ShaderCompilationOptions* options, ShaderMeta* meta);
};

#endif
