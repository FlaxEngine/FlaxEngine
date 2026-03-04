// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUShaderWebGPU.h"
#include "GPUShaderProgramWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include <ThirdParty/LZ4/lz4.h>

GPUConstantBufferWebGPU::GPUConstantBufferWebGPU(GPUDeviceWebGPU* device, uint32 size, const StringView& name) noexcept
    : GPUResourceWebGPU(device, name)
{
    _size = _memoryUsage = size;
    AllocationSize = 0;
}

GPUShaderProgram* GPUShaderWebGPU::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream)
{
    // Extract the SPIR-V shader header from the cache
    SpirvShaderHeader* header = (SpirvShaderHeader*)bytecode.Get();
    bytecode = bytecode.Slice(sizeof(SpirvShaderHeader));

    // Extract the WGSL shader
    BytesContainer wgsl;
    switch (header->Type)
    {
    case SpirvShaderHeader::Types::WGSL:
        wgsl.Link(bytecode);
        break;
    case SpirvShaderHeader::Types::WGSL_LZ4:
    {
        int32 originalSize = *(int32*)bytecode.Get();
        bytecode = bytecode.Slice(sizeof(int32));
        wgsl.Allocate(originalSize);
        const int32 res = LZ4_decompress_safe((const char*)bytecode.Get(), (char*)wgsl.Get(), bytecode.Length(), originalSize);
        if (res <= 0)
        {
            LOG(Error, "Failed to decompress shader");
            return nullptr;
        }
        break;
    }
    default:
        LOG(Error, "Invalid shader program format");
        return nullptr;
    }

    // Create a shader module
    WGPUShaderSourceWGSL shaderCodeDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
    shaderCodeDesc.code = { (const char*)wgsl.Get(), (size_t)wgsl.Length() - 1 };
    WGPUShaderModuleDescriptor shaderDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
#if GPU_ENABLE_RESOURCE_NAMING
    shaderDesc.label = { initializer.Name.Get(), (size_t)initializer.Name.Length() };
#endif
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(_device->Device, &shaderDesc);
    if (!shaderModule)
    {
        LOG(Error, "Failed to create a shader module");
#if GPU_ENABLE_DIAGNOSTICS
        LOG_STR(Warning, String((char*)wgsl.Get(), wgsl.Length()));
#endif
        return nullptr;
    }

    // Create a shader program
    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        GPUVertexLayout* inputLayout, *vertexLayout;
        ReadVertexLayout(stream, inputLayout, vertexLayout);
        shader = New<GPUShaderProgramVSWebGPU>(initializer, inputLayout, vertexLayout, header->DescriptorInfo, shaderModule);
        break;
    }
    case ShaderStage::Pixel:
        shader = New<GPUShaderProgramPSWebGPU>(initializer, header->DescriptorInfo, shaderModule);
        break;
    case ShaderStage::Compute:
        shader = New<GPUShaderProgramCSWebGPU>(initializer, header->DescriptorInfo, shaderModule);
        break;
    }
    return shader;
}

void GPUShaderWebGPU::OnReleaseGPU()
{
    _cbs.Clear();

    GPUShader::OnReleaseGPU();
}

#endif
