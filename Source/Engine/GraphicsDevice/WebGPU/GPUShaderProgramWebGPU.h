// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Dictionary.h"

#if GRAPHICS_API_WEBGPU

/// <summary>
/// Shaders base class for Web GPU backend.
/// </summary>
template<typename BaseType>
class GPUShaderProgramWebGPU : public BaseType
{
public:
    GPUShaderProgramWebGPU(const GPUShaderProgramInitializer& initializer)
    {
        BaseType::Init(initializer);
    }

    ~GPUShaderProgramWebGPU()
    {
    }

public:
    // [BaseType]
    uint32 GetBufferSize() const override
    {
        return 0;
    }
    void* GetBufferHandle() const override
    {
        return nullptr;
    }
};

/// <summary>
/// Vertex Shader for Web GPU backend.
/// </summary>
class GPUShaderProgramVSWebGPU : public GPUShaderProgramWebGPU<GPUShaderProgramVS>
{
public:
    GPUShaderProgramVSWebGPU(const GPUShaderProgramInitializer& initializer, GPUVertexLayout* inputLayout, GPUVertexLayout* vertexLayout, Span<byte> bytecode)
        : GPUShaderProgramWebGPU(initializer)
    {
        InputLayout = inputLayout;
        Layout = vertexLayout;
    }
};

/// <summary>
/// Pixel Shader for Web GPU backend.
/// </summary>
class GPUShaderProgramPSWebGPU : public GPUShaderProgramWebGPU<GPUShaderProgramPS>
{
public:
    GPUShaderProgramPSWebGPU(const GPUShaderProgramInitializer& initializer)
        : GPUShaderProgramWebGPU(initializer)
    {
    }
};

#endif
