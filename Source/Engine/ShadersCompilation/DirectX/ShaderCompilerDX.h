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
    /// <param name="platform">The platform.</param>
    /// <param name="dxcCreateInstanceProc">The custom DXC Compiler factory function.</param>
    ShaderCompilerDX(ShaderProfile profile, PlatformType platform = (PlatformType)0, void* dxcCreateInstanceProc = nullptr);

    /// <summary>
    /// Finalizes an instance of the <see cref="ShaderCompilerDX"/> class.
    /// </summary>
    ~ShaderCompilerDX();

protected:
    virtual void GetArgs(Array<const Char*, InlinedAllocation<250>> args)
    {
    }

    // [ShaderCompiler]
    bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
    bool OnCompileBegin() override;
};

#endif
