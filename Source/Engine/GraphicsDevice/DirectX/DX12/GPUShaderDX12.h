// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "GPUDeviceDX12.h"

/// <summary>
/// Constant Buffer for DirectX 12 backend.
/// </summary>
class GPUConstantBufferDX12 : public GPUResourceDX12<GPUConstantBuffer>
{
public:
    GPUConstantBufferDX12(GPUDeviceDX12* device, uint32 size, const StringView& name) noexcept
        : GPUResourceDX12(device, name)
        , GPUAddress(0)
    {
        _size = size;
    }

public:
    /// <summary>
    /// Last uploaded data address.
    /// </summary>
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
};

/// <summary>
/// Shader for DirectX 12 backend.
/// </summary>
class GPUShaderDX12 : public GPUResourceDX12<GPUShader>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderDX12"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The resource name.</param>
    GPUShaderDX12(GPUDeviceDX12* device, const StringView& name)
        : GPUResourceDX12<GPUShader>(device, name)
    {
    }

protected:
    // [GPUShader]
    GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream) override;
};

#endif
