// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_D3D_SHADER_COMPILER

#include "Engine/ShadersCompilation/ShaderCompiler.h"

/// <summary>
/// Implementation of shaders compiler for DirectX platform using D3DCompiler.
/// </summary>
class ShaderCompilerD3D : public ShaderCompiler
{
private:
    Array<char> _funcNameDefineBuffer;
    uint32 _flags;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilerD3D"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompilerD3D(ShaderProfile profile);

protected:
    // [ShaderCompiler]
    bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
    bool OnCompileBegin() override;
};

#endif
