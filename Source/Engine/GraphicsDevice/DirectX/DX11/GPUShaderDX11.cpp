// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUShaderDX11.h"
#include "GPUShaderProgramDX11.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "../RenderToolsDX.h"

GPUShaderProgram* GPUShaderDX11::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream)
{
    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[VERTEX_SHADER_MAX_INPUT_ELEMENTS];

        // Temporary variables
        byte Type, Format, Index, InputSlot, InputSlotClass;
        uint32 AlignedByteOffset, InstanceDataStepRate;

        // Load Input Layout (it may be empty)
        byte inputLayoutSize;
        stream.ReadByte(&inputLayoutSize);
        ASSERT(inputLayoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
        for (int32 a = 0; a < inputLayoutSize; a++)
        {
            // Read description
            // TODO: maybe use struct and load at once?
            stream.ReadByte(&Type);
            stream.ReadByte(&Index);
            stream.ReadByte(&Format);
            stream.ReadByte(&InputSlot);
            stream.ReadUint32(&AlignedByteOffset);
            stream.ReadByte(&InputSlotClass);
            stream.ReadUint32(&InstanceDataStepRate);

            // Get semantic name
            const char* semanticName = nullptr;
            // TODO: maybe use enum+mapping ?
            switch (Type)
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
                LOG(Fatal, "Invalid vertex shader element semantic type: {0}", Type);
                break;
            }

            // Set data
            inputLayoutDesc[a] =
            {
                semanticName,
                static_cast<UINT>(Index),
                static_cast<DXGI_FORMAT>(Format),
                static_cast<UINT>(InputSlot),
                static_cast<UINT>(AlignedByteOffset),
                static_cast<D3D11_INPUT_CLASSIFICATION>(InputSlotClass),
                static_cast<UINT>(InstanceDataStepRate)
            };
        }

        ID3D11InputLayout* inputLayout = nullptr;
        if (inputLayoutSize > 0)
        {
            // Create input layout
            VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateInputLayout(inputLayoutDesc, inputLayoutSize, cacheBytes, cacheSize, &inputLayout));
        }

        // Create shader
        ID3D11VertexShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateVertexShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramVSDX11>(initializer, buffer, inputLayout, inputLayoutSize);
        break;
    }
    case ShaderStage::Hull:
    {
        // Read control points
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);

        // Create shader
        ID3D11HullShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateHullShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramHSDX11>(initializer, buffer, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        // Create shader
        ID3D11DomainShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateDomainShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramDSDX11>(initializer, buffer);
        break;
    }
    case ShaderStage::Geometry:
    {
        // Create shader
        ID3D11GeometryShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateGeometryShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramGSDX11>(initializer, buffer);
        break;
    }
    case ShaderStage::Pixel:
    {
        // Create shader
        ID3D11PixelShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreatePixelShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramPSDX11>(initializer, buffer);
        break;
    }
    case ShaderStage::Compute:
    {
        // Create shader
        ID3D11ComputeShader* buffer = nullptr;
        VALIDATE_DIRECTX_RESULT(_device->GetDevice()->CreateComputeShader(cacheBytes, cacheSize, nullptr, &buffer));

        // Create object
        shader = New<GPUShaderProgramCSDX11>(initializer, buffer);
        break;
    }
    }
    return shader;
}

GPUConstantBuffer* GPUShaderDX11::CreateCB(const String& name, uint32 size, MemoryReadStream& stream)
{
    ID3D11Buffer* buffer = nullptr;
    uint32 memorySize = 0;
    if (size)
    {
        // Create buffer
        D3D11_BUFFER_DESC cbDesc;
        cbDesc.ByteWidth = Math::AlignUp<uint32>(size, 16);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = 0;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;
        const HRESULT result = _device->GetDevice()->CreateBuffer(&cbDesc, nullptr, &buffer);
        if (FAILED(result))
        {
            LOG_DIRECTX_RESULT(result);
            return nullptr;
        }
        memorySize = cbDesc.ByteWidth;
    }

    return new(_cbs) GPUConstantBufferDX11(_device, name, size, memorySize, buffer);
}

void GPUShaderDX11::OnReleaseGPU()
{
    _cbs.Clear();

    GPUShader::OnReleaseGPU();
}

#endif
