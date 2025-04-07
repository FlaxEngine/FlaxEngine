// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_DX_SHADER_COMPILER

#include "Engine/ShadersCompilation/ShaderCompiler.h"

/// <summary>
/// Implementation of shaders compiler for DirectX platform using https://github.com/microsoft/DirectXShaderCompiler.
/// </summary>
class ShaderCompilerDX : public ShaderCompiler
{
private:
    Array<char> _funcNameDefineBuffer;
    void* _compiler;
    void* _library;
    void* _containerReflection;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilerDX"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompilerDX(ShaderProfile profile);

    /// <summary>
    /// Finalizes an instance of the <see cref="ShaderCompilerDX"/> class.
    /// </summary>
    ~ShaderCompilerDX();

protected:
    // [ShaderCompiler]
    bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
    bool OnCompileBegin() override;
};

#endif
