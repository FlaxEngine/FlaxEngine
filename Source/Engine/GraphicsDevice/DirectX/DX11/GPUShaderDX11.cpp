// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUShaderDX11.h"
#include "GPUShaderProgramDX11.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "../RenderToolsDX.h"

GPUShaderProgram* GPUShaderDX11::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream)
{
    GPUShaderProgram* shader = nullptr;
    HRESULT result;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        // Load Input Layout
        byte inputLayoutSize;
        stream.ReadByte(&inputLayoutSize);
        ASSERT(inputLayoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
        D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
        for (int32 a = 0; a < inputLayoutSize; a++)
        {
            // Read description
            GPUShaderProgramVS::InputElement inputElement;
            stream.Read(inputElement);

            // Get semantic name
            const char* semanticName = nullptr;
            // TODO: maybe use enum+mapping ?
            switch (inputElement.Type)
            {
            case 1:
                semanticName = "POSITION";
                break;
            case 2:
                semanticName = "COLOR";
                break;
            case 3:
                semanticName = "TEXCOORD";
                break;
            case 4:
                semanticName = "NORMAL";
                break;
            case 5:
                semanticName = "TANGENT";
                break;
            case 6:
                semanticName = "BITANGENT";
                break;
            case 7:
                semanticName = "ATTRIBUTE";
                break;
            case 8:
                semanticName = "BLENDINDICES";
                break;
            case 9:
                semanticName = "BLENDWEIGHT";
                break;
            default:
                LOG(Fatal, "Invalid vertex shader element semantic type: {0}", inputElement.Type);
                break;
            }

            // Set data
            inputLayoutDesc[a] =
            {
                semanticName,
                static_cast<UINT>(inputElement.Index),
                static_cast<DXGI_FORMAT>(inputElement.Format),
                static_cast<UINT>(inputElement.InputSlot),
                static_cast<UINT>(inputElement.AlignedByteOffset),
                static_cast<D3D11_INPUT_CLASSIFICATION>(inputElement.InputSlotClass),
                static_cast<UINT>(inputElement.InstanceDataStepRate)
            };
        }

        ID3D11InputLayout* inputLayout = nullptr;
        if (inputLayoutSize > 0)
        {
            // Create input layout
            VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateInputLayout(inputLayoutDesc, inputLayoutSize, cacheBytes, cacheSize, &inputLayout));
        }

        // Create shader
        ID3D11VertexShader* buffer = nullptr;
        result = _device->GetDevice()->CreateVertexShader(cacheBytes, cacheSize, nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramVSDX11>(initializer, buffer, inputLayout, inputLayoutSize);
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
        result = _device->GetDevice()->CreateHullShader(cacheBytes, cacheSize, nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramHSDX11>(initializer, buffer, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        // Create shader
        ID3D11DomainShader* buffer = nullptr;
        result = _device->GetDevice()->CreateDomainShader(cacheBytes, cacheSize, nullptr, &buffer);
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
        result = _device->GetDevice()->CreateGeometryShader(cacheBytes, cacheSize, nullptr, &buffer);
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
        result = _device->GetDevice()->CreatePixelShader(cacheBytes, cacheSize, nullptr, &buffer);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, nullptr);

        // Create object
        shader = New<GPUShaderProgramPSDX11>(initializer, buffer);
        break;
    }
    case ShaderStage::Compute:
    {
        // Create shader
        ID3D11ComputeShader* buffer = nullptr;
        result = _device->GetDevice()->CreateComputeShader(cacheBytes, cacheSize, nullptr, &buffer);
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
