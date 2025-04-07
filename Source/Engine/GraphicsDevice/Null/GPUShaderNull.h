// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/Shaders/GPUShader.h"

/// <summary>
/// Shader for Null backend.
/// </summary>
class GPUShaderNull : public GPUShader
{
protected:

    // [GPUShader]
    GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream) override
    {
        return nullptr;
    }

public:

    // [GPUShader]
    bool Create(MemoryReadStream& stream) override
    {
        return false;
    }
};

#endif
