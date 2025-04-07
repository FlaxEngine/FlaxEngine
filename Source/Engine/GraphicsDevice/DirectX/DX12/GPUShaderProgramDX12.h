// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Types.h"
#include "../IncludeDirectXHeaders.h"

/// <summary>
/// Shaders base class for DirectX 12 backend.
/// </summary>
template<typename BaseType>
class GPUShaderProgramDX12 : public BaseType
{
public:
    GPUShaderProgramDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode)
        : Header(*header)
    {
        BaseType::Init(initializer);
        Bytecode.Copy(bytecode);
    }

public:
    BytesContainer Bytecode;
    DxShaderHeader Header;

public:
    // [BaseType]
    void* GetBufferHandle() const override
    {
        return (void*)Bytecode.Get();
    }
    uint32 GetBufferSize() const override
    {
        return Bytecode.Length();
    }
};

/// <summary>
/// Vertex Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramVSDX12 : public GPUShaderProgramDX12<GPUShaderProgramVS>
{
public:
    GPUShaderProgramVSDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode, GPUVertexLayout* inputLayout, GPUVertexLayout* vertexLayout)
        : GPUShaderProgramDX12(initializer, header, bytecode)
    {
        InputLayout = inputLayout;
        Layout = vertexLayout;
    }
};

#if GPU_ALLOW_TESSELLATION_SHADERS
/// <summary>
/// Hull Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramHSDX12 : public GPUShaderProgramDX12<GPUShaderProgramHS>
{
public:
    GPUShaderProgramHSDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode, int32 controlPointsCount)
        : GPUShaderProgramDX12(initializer, header, bytecode)
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
    GPUShaderProgramDSDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode)
        : GPUShaderProgramDX12(initializer, header, bytecode)
    {
    }
};
#endif

#if GPU_ALLOW_GEOMETRY_SHADERS
/// <summary>
/// Geometry Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramGSDX12 : public GPUShaderProgramDX12<GPUShaderProgramGS>
{
public:
    GPUShaderProgramGSDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode)
        : GPUShaderProgramDX12(initializer, header, bytecode)
    {
    }
};
#endif

/// <summary>
/// Pixel Shader for DirectX 12 backend.
/// </summary>
class GPUShaderProgramPSDX12 : public GPUShaderProgramDX12<GPUShaderProgramPS>
{
public:
    GPUShaderProgramPSDX12(const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode)
        : GPUShaderProgramDX12(initializer, header, bytecode)
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
    GPUShaderProgramCSDX12(GPUDeviceDX12* device, const GPUShaderProgramInitializer& initializer, const DxShaderHeader* header, Span<byte> bytecode)
        : GPUShaderProgramDX12(initializer, header, bytecode)
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
