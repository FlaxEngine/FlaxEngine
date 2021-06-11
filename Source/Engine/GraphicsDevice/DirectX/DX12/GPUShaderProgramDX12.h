// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Types.h"
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


    GPUShaderProgramDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize)
        : Header(*header)
    {
        BaseType::Init(initializer);
        _data.Set(cacheBytes, cacheSize);
    }

public:

    DxShaderHeader Header;

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

    GPUShaderProgramVSDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize, D3D12_INPUT_ELEMENT_DESC* inputLayout, byte inputLayoutSize)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
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

    GPUShaderProgramHSDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize, int32 controlPointsCount)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
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

    GPUShaderProgramDSDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
    {
    }
};

/// <summary>
/// Geometry Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramGSDX12 : public GPUShaderProgramDX12<GPUShaderProgramGS>
{
public:

    GPUShaderProgramGSDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
    {
    }
};

/// <summary>
/// Pixel Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramPSDX12 : public GPUShaderProgramDX12<GPUShaderProgramPS>
{
public:

    GPUShaderProgramPSDX12(const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
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

    GPUShaderProgramCSDX12(GPUDeviceDX12* device, const GPUShaderProgramInitializer& initializer, DxShaderHeader* header, byte* cacheBytes, uint32 cacheSize)
        : GPUShaderProgramDX12(initializer, header, cacheBytes, cacheSize)
        , _device(device)
        , _state(nullptr)
    {
    }
   
    ~GPUShaderProgramCSDX12()
    {
        _device->AddResourceToLateRelease(_state);
    }

public:

    /// <summary>
    /// Gets DirectX 12 compute pipeline state object
    /// </summary>
    FORCE_INLINE ID3D12PipelineState* GetState() const
    {
        return _state;
    }

    /// <summary>
    /// Gets or creates compute pipeline state for that compute shader.
    /// </summary>
    ID3D12PipelineState* GetOrCreateState();
};

#endif
