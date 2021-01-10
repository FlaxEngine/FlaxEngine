// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11

#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "GPUDeviceDX11.h"

/// <summary>
/// Constant Buffer for DirectX 11 backend.
/// </summary>
class GPUConstantBufferDX11 : public GPUResourceDX11<GPUConstantBuffer>
{
private:

    ID3D11Buffer* _resource;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUConstantBufferDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    /// <param name="size">The buffer size (in bytes).</param>
    /// <param name="memorySize">The buffer memory size (in bytes).</param>
    /// <param name="buffer">The buffer.</param>
    GPUConstantBufferDX11(GPUDeviceDX11* device, const String& name, uint32 size, uint32 memorySize, ID3D11Buffer* buffer) noexcept
        : GPUResourceDX11(device, name)
        , _resource(buffer)
    {
        _size = size;
        _memoryUsage = memorySize;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUConstantBufferDX11"/> class.
    /// </summary>
    ~GPUConstantBufferDX11()
    {
        DX_SAFE_RELEASE_CHECK(_resource, 0);
        _memoryUsage = 0;
    }

public:

    /// <summary>
    /// Gets the constant buffer object.
    /// </summary>
    /// <returns>The DirectX buffer object.</returns>
    FORCE_INLINE ID3D11Buffer* GetBuffer() const
    {
        return _resource;
    }

public:

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() override
    {
        return _resource;
    }

public:

    // [GPUResourceDX11]
    void OnReleaseGPU() final override
    {
        DX_SAFE_RELEASE_CHECK(_resource, 0);
    }
};

/// <summary>
/// Shader for DirectX 11 backend.
/// </summary>
class GPUShaderDX11 : public GPUResourceDX11<GPUShader>
{
private:

    Array<GPUConstantBufferDX11, FixedAllocation<MAX_CONSTANT_BUFFER_SLOTS>> _cbs;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderDX11"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The resource name.</param>
    GPUShaderDX11(GPUDeviceDX11* device, const StringView& name)
        : GPUResourceDX11(device, name)
    {
    }

protected:

    // [GPUShader]
    GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream) override;
    GPUConstantBuffer* CreateCB(const String& name, uint32 size, MemoryReadStream& stream) override;
    void OnReleaseGPU() override;

public:

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() final override
    {
        return nullptr;
    }
};

#endif
