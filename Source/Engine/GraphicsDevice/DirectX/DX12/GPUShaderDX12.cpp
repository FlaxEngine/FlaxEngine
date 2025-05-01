// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUShaderDX12.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "GPUShaderProgramDX12.h"
#include "Types.h"
#include "../RenderToolsDX.h"

GPUShaderProgram* GPUShaderDX12::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream)
{
    // Extract the DX shader header from the cache
    DxShaderHeader* header = (DxShaderHeader*)bytecode.Get();
    bytecode = bytecode.Slice(sizeof(DxShaderHeader));

    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        GPUVertexLayout* inputLayout, *vertexLayout;
        ReadVertexLayout(stream, inputLayout, vertexLayout);
        shader = New<GPUShaderProgramVSDX12>(initializer, header, bytecode, inputLayout, vertexLayout);
        break;
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    case ShaderStage::Hull:
    {
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);
        shader = New<GPUShaderProgramHSDX12>(initializer, header, bytecode, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        shader = New<GPUShaderProgramDSDX12>(initializer, header, bytecode);
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
        shader = New<GPUShaderProgramGSDX12>(initializer, header, bytecode);
        break;
    }
#endif
    case ShaderStage::Pixel:
    {
        shader = New<GPUShaderProgramPSDX12>(initializer, header, bytecode);
        break;
    }
    case ShaderStage::Compute:
    {
        shader = New<GPUShaderProgramCSDX12>(_device, initializer, header, bytecode);
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
