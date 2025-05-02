// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_SHADER_COMPILER

#include "ShaderCompiler.h"

class Asset;

/// <summary>
/// Shaders compilation service allows to compile shader source code for a desire platform. Supports multi-threading.
/// </summary>
class FLAXENGINE_API ShadersCompilation
{
public:
    /// <summary>
    /// Compiles the shader.
    /// </summary>
    /// <param name="options">Compilation options</param>
    /// <returns>True if failed, otherwise false</returns>
    static bool Compile(ShaderCompilationOptions& options);

    /// <summary>
    /// Registers shader asset for the automated reloads on source includes changes.
    /// </summary
    /// <param name="asset">The asset.</param>
    /// <param name="includedPath">The included file path.</param>
    static void RegisterForShaderReloads(Asset* asset, const String& includedPath);

    /// <summary>
    /// Unregisters shader asset from the automated reloads on source includes changes.
    /// </summary>
    /// <param name="asset">The asset.</param>
    static void UnregisterForShaderReloads(Asset* asset);

    /// <summary>
    /// Reads the included shader files stored in the shader cache data.
    /// </summary>
    /// <param name="shaderCache">The shader cache data.</param>
    /// <param name="shaderCacheLength">The shader cache data length (in bytes).</param>
    /// <param name="includes">The output included.</param>
    static void ExtractShaderIncludes(byte* shaderCache, int32 shaderCacheLength, Array<String>& includes);

    // Resolves shader path name into absolute file path. Resolves './<ProjectName>/ShaderFile.hlsl' cases into a full path.
    static String ResolveShaderPath(StringView path);
    // Compacts the full shader file path into portable format with project name prefix such as './<ProjectName>/ShaderFile.hlsl'.
    static String CompactShaderPath(StringView path);

private:

    static ShaderCompiler* CreateCompiler(ShaderProfile profile);
    static ShaderCompiler* RequestCompiler(ShaderProfile profile);
    static void FreeCompiler(ShaderCompiler* compiler);
};

#endif
