// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "GPUDeviceWebGPU.h"

/// <summary>
/// Constant Buffer for Web GPU backend.
/// </summary>
class GPUConstantBufferWebGPU : public GPUResourceWebGPU<GPUConstantBuffer>
{
public:
    GPUConstantBufferWebGPU(GPUDeviceWebGPU* device, uint32 size, WGPUBuffer buffer, const StringView& name) noexcept;
    ~GPUConstantBufferWebGPU();

public:
    WGPUBuffer Buffer;

public:
    // [GPUResourceWebGPU]
    void OnReleaseGPU() final override;
};

/// <summary>
/// Shader for Web GPU backend.
/// </summary>
class GPUShaderWebGPU : public GPUResourceWebGPU<GPUShader>
{
private:
    Array<GPUConstantBufferWebGPU, FixedAllocation<GPU_MAX_CB_BINDED>> _cbs;

public:
    GPUShaderWebGPU(GPUDeviceWebGPU* device, const StringView& name)
        : GPUResourceWebGPU(device, name)
    {
    }

protected:
    // [GPUShader]
    GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream) override;
    void OnReleaseGPU() override;
};

#endif
