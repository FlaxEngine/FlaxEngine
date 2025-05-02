// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_VK_SHADER_COMPILER

#include "Engine/ShadersCompilation/ShaderCompiler.h"

/// <summary>
/// Implementation of shaders compiler for Vulkan rendering backend.
/// </summary>
class ShaderCompilerVulkan : public ShaderCompiler
{
private:
    Array<char> _funcNameDefineBuffer;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilerVulkan"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompilerVulkan(ShaderProfile profile);

    /// <summary>
    /// Finalizes an instance of the <see cref="ShaderCompilerVulkan"/> class.
    /// </summary>
    ~ShaderCompilerVulkan();

protected:
    // [ShaderCompiler]
    bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
    bool OnCompileBegin() override;
};

#endif
