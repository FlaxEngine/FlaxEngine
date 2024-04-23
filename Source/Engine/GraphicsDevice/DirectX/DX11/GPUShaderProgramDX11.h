// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
    GPUShaderProgramDX11(const GPUShaderProgramInitializer& initializer, BufferType* buffer)
        : _buffer(buffer)
    {
        BaseType::Init(initializer);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgramDX11"/> class.
    /// </summary>
    ~GPUShaderProgramDX11()
    {
        DX_SAFE_RELEASE_CHECK(_buffer, 0);
    }

public:

    /// <summary>
    /// Gets DirectX 11 buffer handle.
    /// </summary>
    /// <returns>The DirectX 11 buffer.</returns>
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

    byte _inputLayoutSize;
    ID3D11InputLayout* _inputLayout;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramVSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
    /// <param name="inputLayout">The input layout.</param>
    /// <param name="inputLayoutSize">Size of the input layout.</param>
    GPUShaderProgramVSDX11(const GPUShaderProgramInitializer& initializer, ID3D11VertexShader* buffer, ID3D11InputLayout* inputLayout, byte inputLayoutSize)
        : GPUShaderProgramDX11(initializer, buffer)
        , _inputLayoutSize(inputLayoutSize)
        , _inputLayout(inputLayout)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgramVSDX11"/> class.
    /// </summary>
    ~GPUShaderProgramVSDX11()
    {
        DX_SAFE_RELEASE_CHECK(_inputLayout, 0);
    }

public:

    /// <summary>
    /// Gets the DirectX 11 input layout handle
    /// </summary>
    /// <returns>DirectX 11 input layout</returns>
    FORCE_INLINE ID3D11InputLayout* GetInputLayoutDX11() const
    {
        return _inputLayout;
    }

public:

    // [GPUShaderProgramDX11]
    void* GetInputLayout() const override
    {
        return (void*)_inputLayout;
    }

    byte GetInputLayoutSize() const override
    {
        return _inputLayoutSize;
    }
};

#if GPU_ALLOW_TESSELLATION_SHADERS
/// <summary>
/// Hull Shader for DirectX 11 backend.
/// </summary>
class GPUShaderProgramHSDX11 : public GPUShaderProgramDX11<GPUShaderProgramHS, ID3D11HullShader>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramHSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
    /// <param name="controlPointsCount">The control points used by the hull shader for processing.</param>
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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramDSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramGSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramPSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
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

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramCSDX11"/> class.
    /// </summary>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="buffer">The shader buffer object.</param>
    GPUShaderProgramCSDX11(const GPUShaderProgramInitializer& initializer, ID3D11ComputeShader* buffer)
        : GPUShaderProgramDX11(initializer, buffer)
    {
    }
};

#endif
