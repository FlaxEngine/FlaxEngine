// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_WEBGPU_SHADER_COMPILER

#include "Engine/ShadersCompilation/Vulkan/ShaderCompilerVulkan.h"

/// <summary>
/// Implementation of shaders compiler for Web GPU rendering backend.
/// </summary>
class ShaderCompilerWebGPU : public ShaderCompilerVulkan
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilerWebGPU"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompilerWebGPU(ShaderProfile profile);

protected:
    bool OnCompileBegin() override;
    void InitParsing(ShaderCompilationContext* context, glslang::TShader& shader) override;
    void InitCodegen(ShaderCompilationContext* context, glslang::SpvOptions& spvOptions) override;
    bool Write(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, struct SpirvShaderHeader& header, std::vector<unsigned>& spirv) override;
};

#endif
