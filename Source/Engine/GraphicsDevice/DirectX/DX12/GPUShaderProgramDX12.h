// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "../IncludeDirectXHeaders.h"

/// <summary>
/// Shaders base class for DirectX 12 backend.
/// </summary>
template<typename BaseType>
class GPUShaderProgramDX12 : public BaseType
{
protected:

    Array<byte> _data;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    GPUShaderProgramDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize)
    {
        BaseType::Init(initializer);
        _data.Set(cacheBytes, cacheSize);
    }

public:

    // [BaseType]
    void* GetBufferHandle() const override
    {
        return (void*)_data.Get();
    }

    uint32 GetBufferSize() const override
    {
        return _data.Count();
    }
};

/// <summary>
/// Vertex Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramVSDX12 : public GPUShaderProgramDX12<GPUShaderProgramVS>
{
private:

    byte _inputLayoutSize;
    D3D12_INPUT_ELEMENT_DESC _inputLayout[VERTEX_SHADER_MAX_INPUT_ELEMENTS];

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramVSDX12"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    /// <param name="inputLayout">The input layout description.</param>
    /// <param name="inputLayoutSize">The input layout description size.</param>
    GPUShaderProgramVSDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, D3D12_INPUT_ELEMENT_DESC* inputLayout, byte inputLayoutSize)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
        , _inputLayoutSize(inputLayoutSize)
    {
        for (byte i = 0; i < inputLayoutSize; i++)
            _inputLayout[i] = inputLayout[i];
    }

public:

    // [GPUShaderProgramDX12]
    void* GetInputLayout() const override
    {
        return (void*)_inputLayout;
    }

    byte GetInputLayoutSize() const override
    {
        return _inputLayoutSize;
    }
};

/// <summary>
/// Hull Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramHSDX12 : public GPUShaderProgramDX12<GPUShaderProgramHS>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramHSDX12"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    /// <param name="controlPointsCount">The control points used by the hull shader for processing.</param>
    GPUShaderProgramHSDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, int32 controlPointsCount)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
    {
        _controlPointsCount = controlPointsCount;
    }
};

/// <summary>
/// Domain Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramDSDX12 : public GPUShaderProgramDX12<GPUShaderProgramDS>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramDSDX12"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    GPUShaderProgramDSDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
    {
    }
};

/// <summary>
/// Geometry Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramGSDX12 : public GPUShaderProgramDX12<GPUShaderProgramGS>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramGSDX12"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    GPUShaderProgramGSDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
    {
    }
};

/// <summary>
/// Pixel Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramPSDX12 : public GPUShaderProgramDX12<GPUShaderProgramPS>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramPSDX12"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    GPUShaderProgramPSDX12(const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
    {
    }
};

/// <summary>
/// Compute Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramCSDX12 : public GPUShaderProgramDX12<GPUShaderProgramCS>
{
private:

    GPUDeviceDX12* _device;
    Array<byte> _data;
    ID3D12PipelineState* _state;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramCSDX12"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="cacheBytes">The shader data.</param>
    /// <param name="cacheSize">The shader data size.</param>
    GPUShaderProgramCSDX12(GPUDeviceDX12* device, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, cacheBytes, cacheSize)
        , _device(device)
        , _state(nullptr)
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~GPUShaderProgramCSDX12()
    {
        _device->AddResourceToLateRelease(_state);
    }

public:

    /// <summary>
    /// Gets DirectX 12 compute pipeline state object
    /// </summary>
    /// <returns>DirectX 12 compute pipeline state object</returns>
    FORCE_INLINE ID3D12PipelineState* GetState() const
    {
        return _state;
    }

    /// <summary>
    /// Gets or creates compute pipeline state for that compute shader.
    /// </summary>
    /// <returns>DirectX 12 compute pipeline state object</returns>
    ID3D12PipelineState* GetOrCreateState();
};

#endif
