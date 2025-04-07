// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Engine/Core/Types/DataContainer.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// Shaders base class for DirectX 11 backend.
/// </summary>
template<typename BaseType, typename BufferType>
class GPUShaderProgramDX11 : public BaseType
{
protected:
    BufferType* _buffer;

public:
    GPUShaderProgramDX11(const GPUShaderProgramInitializer& initializer, BufferType* buffer)
        : _buffer(buffer)
    {
        BaseType::Init(initializer);
#if GPU_ENABLE_RESOURCE_NAMING
        SetDebugObjectName(buffer, initializer.Name.Get(), initializer.Name.Length());
#endif
    }

    ~GPUShaderProgramDX11()
    {
        DX_SAFE_RELEASE_CHECK(_buffer, 0);
    }

    FORCE_INLINE BufferType* GetBufferHandleDX11() const
    {
        return _buffer;
    }

public:
    // [BaseType]
    uint32 GetBufferSize() const override
    {
        return 0;
    }
    void* GetBufferHandle() const override
    {
        return _buffer;
    }
};

/// <summary>
/// Vertex Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramVSDX11 : public GPUShaderProgramDX11<GPUShaderProgramVS, ID3D11VertexShader>
{
private:
    Dictionary<class GPUVertexLayoutDX11*, ID3D11InputLayout*> _cache;

public:
    GPUShaderProgramVSDX11(const GPUShaderProgramInitializer& initializer, ID3D11VertexShader* buffer, GPUVertexLayout* inputLayout, GPUVertexLayout* vertexLayout, Span<byte> bytecode)
        : GPUShaderProgramDX11(initializer, buffer)
    {
        InputLayout = inputLayout;
        Layout = vertexLayout;
        Bytecode.Copy(bytecode);
    }

    ~GPUShaderProgramVSDX11();

    BytesContainer Bytecode;

    ID3D11InputLayout* GetInputLayout(class GPUVertexLayoutDX11* vertexLayout);
};

#if GPU_ALLOW_TESSELLATION_SHADERS
/// <summary>
/// Hull Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramHSDX11 : public GPUShaderProgramDX11<GPUShaderProgramHS, ID3D11HullShader>
{
public:
    GPUShaderProgramHSDX11(const GPUShaderProgramInitializer& initializer, ID3D11HullShader* buffer, int32 controlPointsCount)
        : GPUShaderProgramDX11(initializer, buffer)
    {
        _controlPointsCount = controlPointsCount;
    }
};

/// <summary>
/// Domain Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramDSDX11 : public GPUShaderProgramDX11<GPUShaderProgramDS, ID3D11DomainShader>
{
public:
    GPUShaderProgramDSDX11(const GPUShaderProgramInitializer& initializer, ID3D11DomainShader* buffer)
        : GPUShaderProgramDX11(initializer, buffer)
    {
    }
};
#endif

#if GPU_ALLOW_GEOMETRY_SHADERS
/// <summary>
/// Geometry Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramGSDX11 : public GPUShaderProgramDX11<GPUShaderProgramGS, ID3D11GeometryShader>
{
public:
    GPUShaderProgramGSDX11(const GPUShaderProgramInitializer& initializer, ID3D11GeometryShader* buffer)
        : GPUShaderProgramDX11(initializer, buffer)
    {
    }
};
#endif

/// <summary>
/// Pixel Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramPSDX11 : public GPUShaderProgramDX11<GPUShaderProgramPS, ID3D11PixelShader>
{
public:
    GPUShaderProgramPSDX11(const GPUShaderProgramInitializer& initializer, ID3D11PixelShader* buffer)
        : GPUShaderProgramDX11(initializer, buffer)
    {
    }
};

/// <summary>
/// Compute Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramCSDX11 : public GPUShaderProgramDX11<GPUShaderProgramCS, ID3D11ComputeShader>
{
public:
    GPUShaderProgramCSDX11(const GPUShaderProgramInitializer& initializer, ID3D11ComputeShader* buffer)
        : GPUShaderProgramDX11(initializer, buffer)
    {
    }
};

#endif
