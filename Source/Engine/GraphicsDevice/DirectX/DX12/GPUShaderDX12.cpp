// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUShaderDX12.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "GPUShaderProgramDX12.h"
#include "Types.h"
#include "../RenderToolsDX.h"

GPUShaderProgram* GPUShaderDX12::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream)
{
    // Extract the DX shader header from the cache
    DxShaderHeader* header = (DxShaderHeader*)cacheBytes;
    cacheBytes += sizeof(DxShaderHeader);
    cacheSize -= sizeof(DxShaderHeader);

    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        // Load Input Layout (it may be empty)
        byte inputLayoutSize;
        stream.ReadByte(&inputLayoutSize);
        ASSERT(inputLayoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
        D3D12_INPUT_ELEMENT_DESC inputLayout[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
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
            inputLayout[a] =
            {
                semanticName,
                static_cast<UINT>(inputElement.Index),
                static_cast<DXGI_FORMAT>(inputElement.Format),
                static_cast<UINT>(inputElement.InputSlot),
                static_cast<UINT>(inputElement.AlignedByteOffset),
                static_cast<D3D12_INPUT_CLASSIFICATION>(inputElement.InputSlotClass),
                static_cast<UINT>(inputElement.InstanceDataStepRate)
            };
        }

        // Create object
        shader = New<GPUShaderProgramVSDX12>(initializer, header, cacheBytes, cacheSize, inputLayout, inputLayoutSize);
        break;
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    case ShaderStage::Hull:
    {
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);
        shader = New<GPUShaderProgramHSDX12>(initializer, header, cacheBytes, cacheSize, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        shader = New<GPUShaderProgramDSDX12>(initializer, header, cacheBytes, cacheSize);
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
        shader = New<GPUShaderProgramGSDX12>(initializer, header, cacheBytes, cacheSize);
        break;
    }
#endif
    case ShaderStage::Pixel:
    {
        shader = New<GPUShaderProgramPSDX12>(initializer, header, cacheBytes, cacheSize);
        break;
    }
    case ShaderStage::Compute:
    {
        shader = New<GPUShaderProgramCSDX12>(_device, initializer, header, cacheBytes, cacheSize);
        break;
    }
    }
    return shader;
}

ID3D12PipelineState* GPUShaderProgramCSDX12::GetOrCreateState()
{
    if (_state)
        return _state;

    // Create description
    D3D12_COMPUTE_PIPELINE_STATE_DESC psDesc;
    Platform::MemoryClear(&psDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
    psDesc.pRootSignature = _device->GetRootSignature();

    // Shader
    psDesc.CS = { GetBufferHandle(), GetBufferSize() };

    // Create object
    const HRESULT result = _device->GetDevice()->CreateComputePipelineState(&psDesc, IID_PPV_ARGS(&_state));
    LOG_DIRECTX_RESULT(result);

    return _state;
}

#endif
