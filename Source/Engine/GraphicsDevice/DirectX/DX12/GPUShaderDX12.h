// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUConstantBufferDX12"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="size">The buffer size (in bytes).</param>
    GPUConstantBufferDX12(GPUDeviceDX12* device, uint32 size) noexcept
        : GPUResourceDX12(device, String::Empty)
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
private:

    Array<GPUConstantBufferDX12, FixedAllocation<MAX_CONSTANT_BUFFER_SLOTS>> _cbs;

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
    GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream) override;
    GPUConstantBuffer* CreateCB(const String& name, uint32 size, MemoryReadStream& stream) override;
    void OnReleaseGPU() override;
};

#endif
