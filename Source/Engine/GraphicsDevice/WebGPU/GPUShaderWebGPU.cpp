// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUShaderWebGPU.h"
#include "GPUShaderProgramWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "Engine/Serialization/MemoryReadStream.h"

GPUConstantBufferWebGPU::GPUConstantBufferWebGPU(GPUDeviceWebGPU* device, uint32 size, WGPUBuffer buffer, const StringView& name) noexcept
    : GPUResourceWebGPU(device, name)
{
    _size = _memoryUsage = size;
    Buffer = buffer;
}

GPUConstantBufferWebGPU::~GPUConstantBufferWebGPU()
{
    if (Buffer)
        wgpuBufferRelease(Buffer);
}

void GPUConstantBufferWebGPU::OnReleaseGPU()
{
    if (Buffer)
    {
        wgpuBufferRelease(Buffer);
        Buffer = nullptr;
    }
}

GPUShaderProgram* GPUShaderWebGPU::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream)
{
    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        GPUVertexLayout* inputLayout, *vertexLayout;
        ReadVertexLayout(stream, inputLayout, vertexLayout);
        MISSING_CODE("create vertex shader");
        shader = New<GPUShaderProgramVSWebGPU>(initializer, inputLayout, vertexLayout, bytecode);
        break;
    }
    case ShaderStage::Pixel:
    {
        MISSING_CODE("create pixel shader");
        shader = New<GPUShaderProgramPSWebGPU>(initializer);
        break;
    }
    }
    return shader;
}

void GPUShaderWebGPU::OnReleaseGPU()
{
    _cbs.Clear();

    GPUShader::OnReleaseGPU();
}

#endif
