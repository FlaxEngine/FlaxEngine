// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUShaderDX11.h"
#include "GPUShaderProgramDX11.h"
#include "GPUVertexLayoutDX11.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "../RenderToolsDX.h"

GPUShaderProgramVSDX11::~GPUShaderProgramVSDX11()
{
    for (const auto& e : _cache)
    {
        if (e.Value)
            e.Value->Release();
    }
}

ID3D11InputLayout* GPUShaderProgramVSDX11::GetInputLayout(GPUVertexLayoutDX11* vertexLayout)
{
    ID3D11InputLayout* inputLayout = nullptr;
    if (!vertexLayout)
        vertexLayout = (GPUVertexLayoutDX11*)Layout;
    if (!_cache.TryGet(vertexLayout, inputLayout))
    {
        if (vertexLayout && vertexLayout->InputElementsCount)
        {
            auto mergedVertexLayout = (GPUVertexLayoutDX11*)GPUVertexLayout::Merge(vertexLayout, Layout);
            LOG_DIRECTX_RESULT(vertexLayout->GetDevice()->GetDevice()->CreateInputLayout(mergedVertexLayout->InputElements, mergedVertexLayout->InputElementsCount, Bytecode.Get(), Bytecode.Length(), &inputLayout));
        }
        _cache.Add(vertexLayout, inputLayout);
    }
    return inputLayout;
}

GPUShaderProgram* GPUShaderDX11::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream)
{
    GPUShaderProgram* shader = nullptr;
    HRESULT result;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        // Load Input Layout
        GPUVertexLayout* vertexLayout = ReadVertexLayout(stream);

        // Create shader
        ID3D11VertexShader* buffer = nullptr;
        result = _device->GetDevice()->CreateVertexShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramVSDX11>(initializer, buffer, vertexLayout, bytecode);
        break;
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    case ShaderStage::Hull:
    {
        // Read control points
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);

        // Create shader
        ID3D11HullShader* buffer = nullptr;
        result = _device->GetDevice()->CreateHullShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramHSDX11>(initializer, buffer, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        // Create shader
        ID3D11DomainShader* buffer = nullptr;
        result = _device->GetDevice()->CreateDomainShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramDSDX11>(initializer, buffer);
        break;
    }
#else
    case ShaderStage::Hull:
    {
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);
        break;
    }
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    case ShaderStage::Geometry:
    {
        // Create shader
        ID3D11GeometryShader* buffer = nullptr;
        result = _device->GetDevice()->CreateGeometryShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramGSDX11>(initializer, buffer);
        break;
    }
#endif
    case ShaderStage::Pixel:
    {
        // Create shader
        ID3D11PixelShader* buffer = nullptr;
        result = _device->GetDevice()->CreatePixelShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramPSDX11>(initializer, buffer);
        break;
    }
    case ShaderStage::Compute:
    {
        // Create shader
        ID3D11ComputeShader* buffer = nullptr;
        result = _device->GetDevice()->CreateComputeShader(bytecode.Get(), bytecode.Length(), nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramCSDX11>(initializer, buffer);
        break;
    }
    }
    return shader;
}

void GPUShaderDX11::OnReleaseGPU()
{
    _cbs.Clear();

    GPUShader::OnReleaseGPU();
}

#endif
