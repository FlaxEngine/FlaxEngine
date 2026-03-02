// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUDeviceWebGPU.h"
#include "GPUShaderWebGPU.h"
#include "GPUContextWebGPU.h"
#include "GPUPipelineStateWebGPU.h"
#include "GPUTextureWebGPU.h"
#include "GPUBufferWebGPU.h"
#include "GPUSamplerWebGPU.h"
#include "GPUAdapterWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "GPUSwapChainWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#if BUILD_DEBUG
#include "Engine/Core/Collections/Sorting.h"
#endif
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include <emscripten/emscripten.h>

GPUVertexLayoutWebGPU::GPUVertexLayoutWebGPU(GPUDeviceWebGPU* device, const Elements& elements, bool explicitOffsets)
    : GPUResourceBase<GPUDeviceWebGPU, GPUVertexLayout>(device, StringView::Empty)
{
    SetElements(elements, explicitOffsets);
}

GPUDataUploaderWebGPU::Allocation GPUDataUploaderWebGPU::Allocate(uint32 size, WGPUBufferUsage usage, uint32 alignment)
{
    // Find a free buffer from the current frame
    for (auto& e : _entries)
    {
        uint32 alignedOffset = Math::AlignUp(e.ActiveOffset, alignment);
        if (e.ActiveFrame == _frame && e.Usage == usage && alignedOffset + size <= e.Size)
        {
            e.ActiveOffset = alignedOffset + size;
            return { e.Buffer, alignedOffset };
        }
    }

    // Find an unused buffer from the old frames
    for (auto& e : _entries)
    {
        if (e.ActiveFrame < _frame - 3 && e.Usage == usage && size <= e.Size)
        {
            e.ActiveOffset = size;
            e.ActiveFrame = _frame;
            return { e.Buffer, 0 };
        }
    }

    // Allocate a new buffer
    {
        WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
        if (usage & WGPUBufferUsage_Uniform)
            desc.label = WEBGPU_STR("Upload Uniforms");
        else
            desc.label = WEBGPU_STR("Upload Buffer");
#endif
        desc.size = Math::Max<uint32>(16 * 1024, Math::RoundUpToPowerOf2(size)); // Allocate larger pages for good suballocations
        desc.usage = usage;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(_device, &desc);
        if (buffer == nullptr)
        {
            LOG(Error, "Failed to create buffer of size {} bytes", size);
            return { nullptr, 0 };
        }
        _entries.Insert(0, { buffer, (uint32)desc.size, size, _frame, desc.usage });
        PROFILE_MEM_INC(GraphicsBuffers, desc.usage);
        return { buffer, 0 };
    }
}

void GPUDataUploaderWebGPU::DrawBegin()
{
    // Free old buffers and recycle unused ones
    uint64 frame = Engine::FrameCount;
    for (int32 i = _entries.Count() - 1; i >= 0; i--)
    {
        auto& e = _entries[i];
        if (frame - e.ActiveFrame > 100)
        {
            wgpuBufferRelease(e.Buffer);
            PROFILE_MEM_DEC(GraphicsBuffers, e.Size);
            _entries.RemoveAt(i);
        }
    }
    _frame = frame;
}

void GPUDataUploaderWebGPU::ReleaseGPU()
{
    // Free data
    for (auto& e : _entries)
    {
        wgpuBufferRelease(e.Buffer);
        PROFILE_MEM_DEC(GraphicsBuffers, e.Size);
    }
    _entries.Clear();
}

GPUDeviceWebGPU::GPUDeviceWebGPU(WGPUInstance instance, GPUAdapterWebGPU* adapter)
    : GPUDevice(RendererType::WebGPU, ShaderProfile::WebGPU)
    , Adapter(adapter)
    , WebGPUInstance(instance)
{
}

bool GPUDeviceWebGPU::Init()
{
    // Init device limits
    Array<WGPUFeatureName, FixedAllocation<32>> features;
#define CHECK_FEATURE(feature) if (wgpuAdapterHasFeature(Adapter->Adapter, feature)) features.Add(feature)
    CHECK_FEATURE(WGPUFeatureName_CoreFeaturesAndLimits);
    CHECK_FEATURE(WGPUFeatureName_DepthClipControl);
    CHECK_FEATURE(WGPUFeatureName_Depth32FloatStencil8);
    CHECK_FEATURE(WGPUFeatureName_TextureCompressionBC);
    CHECK_FEATURE(WGPUFeatureName_TextureCompressionBCSliced3D);
    CHECK_FEATURE(WGPUFeatureName_TextureCompressionETC2);
    CHECK_FEATURE(WGPUFeatureName_TextureCompressionASTC);
    CHECK_FEATURE(WGPUFeatureName_TextureCompressionASTCSliced3D);
    CHECK_FEATURE(WGPUFeatureName_TimestampQuery);
    CHECK_FEATURE(WGPUFeatureName_IndirectFirstInstance);
    CHECK_FEATURE(WGPUFeatureName_RG11B10UfloatRenderable);
    CHECK_FEATURE(WGPUFeatureName_BGRA8UnormStorage);
    CHECK_FEATURE(WGPUFeatureName_Float32Filterable);
    CHECK_FEATURE(WGPUFeatureName_Float32Blendable);
    CHECK_FEATURE(WGPUFeatureName_ClipDistances);
    CHECK_FEATURE(WGPUFeatureName_TextureFormatsTier1);
    CHECK_FEATURE(WGPUFeatureName_TextureFormatsTier2);
    CHECK_FEATURE(WGPUFeatureName_PrimitiveIndex);
    CHECK_FEATURE(WGPUFeatureName_MultiDrawIndirect);
#undef CHECK_FEATURE
#if BUILD_DEBUG
    LOG(Info, "WebGPU features:");
#define LOG_FEATURE(feature) if (features.Contains(feature)) LOG(Info, " > {}", TEXT(#feature))
    LOG_FEATURE(WGPUFeatureName_CoreFeaturesAndLimits);
    LOG_FEATURE(WGPUFeatureName_DepthClipControl);
    LOG_FEATURE(WGPUFeatureName_Depth32FloatStencil8);
    LOG_FEATURE(WGPUFeatureName_TextureCompressionBC);
    LOG_FEATURE(WGPUFeatureName_TextureCompressionBCSliced3D);
    LOG_FEATURE(WGPUFeatureName_TextureCompressionETC2);
    LOG_FEATURE(WGPUFeatureName_TextureCompressionASTC);
    LOG_FEATURE(WGPUFeatureName_TextureCompressionASTCSliced3D);
    LOG_FEATURE(WGPUFeatureName_TimestampQuery);
    LOG_FEATURE(WGPUFeatureName_IndirectFirstInstance);
    LOG_FEATURE(WGPUFeatureName_RG11B10UfloatRenderable);
    LOG_FEATURE(WGPUFeatureName_BGRA8UnormStorage);
    LOG_FEATURE(WGPUFeatureName_Float32Filterable);
    LOG_FEATURE(WGPUFeatureName_Float32Blendable);
    LOG_FEATURE(WGPUFeatureName_ClipDistances);
    LOG_FEATURE(WGPUFeatureName_TextureFormatsTier1);
    LOG_FEATURE(WGPUFeatureName_TextureFormatsTier2);
    LOG_FEATURE(WGPUFeatureName_PrimitiveIndex);
    LOG_FEATURE(WGPUFeatureName_MultiDrawIndirect);
#undef LOG_FEATURE
#endif
    for (int32 i = 0; i < (int32)PixelFormat::MAX; i++)
        FeaturesPerFormat[i] = FormatFeatures(MSAALevel::None, FormatSupport::None);
    WGPULimits limits = WGPU_LIMITS_INIT;
    if (wgpuAdapterGetLimits(Adapter->Adapter, &limits) == WGPUStatus_Success)
    {
        MinUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment;
        Limits.HasDrawIndirect = true;
        Limits.HasDepthAsSRV = true;
        Limits.HasReadOnlyDepth = true;
        Limits.HasDepthClip = features.Contains(WGPUFeatureName_DepthClipControl);
        Limits.HasReadOnlyDepth = true;
        Limits.MaximumSamplerAnisotropy = 4;
        Limits.MaximumTexture1DSize = Math::Min<int32>(GPU_MAX_TEXTURE_SIZE, limits.maxTextureDimension1D);
        Limits.MaximumTexture2DSize = Math::Min<int32>(GPU_MAX_TEXTURE_SIZE, limits.maxTextureDimension2D);
        Limits.MaximumTexture3DSize = Math::Min<int32>(GPU_MAX_TEXTURE_SIZE, limits.maxTextureDimension3D);
        Limits.MaximumMipLevelsCount = Math::Min<int32>(GPU_MAX_TEXTURE_MIP_LEVELS, (int32)log2(limits.maxTextureDimension2D));
        Limits.MaximumTexture1DArraySize = Limits.MaximumTexture2DArraySize = Math::Min<int32>(GPU_MAX_TEXTURE_ARRAY_SIZE, limits.maxTextureArrayLayers);
        if (limits.maxTextureArrayLayers >= 6)
            Limits.MaximumTextureCubeSize = Limits.MaximumTexture2DSize;

        // Formats support based on: https://gpuweb.github.io/gpuweb/#plain-color-formats
        auto supportsBuffer =
                FormatSupport::Buffer |
                FormatSupport::InputAssemblyIndexBuffer |
                FormatSupport::InputAssemblyVertexBuffer;
        auto supportsTexture =
                FormatSupport::Texture1D |
                FormatSupport::Texture2D |
                FormatSupport::Texture3D |
                FormatSupport::TextureCube |
                FormatSupport::ShaderLoad |
                FormatSupport::ShaderSample |
                FormatSupport::ShaderSampleComparison |
                FormatSupport::ShaderGather |
                FormatSupport::ShaderGatherComparison |
                FormatSupport::Mip;
        auto supportsRender =
                FormatSupport::RenderTarget |
                FormatSupport::Blendable;
        auto supportsDepth =
                FormatSupport::DepthStencil;
        auto supportsMultisampling =
                FormatSupport::MultisampleRenderTarget |
                FormatSupport::MultisampleLoad;
        auto supportsMSAA =
                FormatSupport::MultisampleRenderTarget |
                FormatSupport::MultisampleResolve |
                FormatSupport::MultisampleLoad;

#define ADD_FORMAT(pixelFormat, support)
        FeaturesPerFormat[(int32)PixelFormat::R8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8_UInt].Support |= supportsBuffer | supportsTexture | supportsRender;
        FeaturesPerFormat[(int32)PixelFormat::R8_SInt].Support |= supportsBuffer | supportsTexture | supportsRender;
        FeaturesPerFormat[(int32)PixelFormat::R16_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16_Float].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_UNorm].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_SNorm].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_Float].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UNorm_sRGB].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SNorm].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::B8G8R8A8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R11G11B10_Float].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R9G9B9E5_SharedExp].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_Float].Support |= supportsBuffer | supportsTexture | supportsRender;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::D16_UNorm].Support |= supportsBuffer | supportsTexture | supportsDepth;
        FeaturesPerFormat[(int32)PixelFormat::D24_UNorm_S8_UInt].Support |= supportsBuffer | supportsTexture | supportsDepth;
        FeaturesPerFormat[(int32)PixelFormat::D32_Float].Support |= supportsBuffer | supportsTexture | supportsDepth;
        if (features.Contains(WGPUFeatureName_CoreFeaturesAndLimits))
        {
            FeaturesPerFormat[(int32)PixelFormat::B8G8R8A8_UNorm_sRGB].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
            FeaturesPerFormat[(int32)PixelFormat::R8_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R8_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R8G8_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R8G8_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SInt].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_Float].Support |= supportsMSAA;
            FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UInt].Support |= supportsMultisampling;
        }
        if (features.Contains(WGPUFeatureName_TextureFormatsTier1))
        {
            FeaturesPerFormat[(int32)PixelFormat::R8_SNorm].Support |= supportsBuffer | supportsTexture;
            FeaturesPerFormat[(int32)PixelFormat::R16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
            FeaturesPerFormat[(int32)PixelFormat::R16_SNorm].Support |= supportsBuffer | supportsTexture;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_SNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling;
        }
        if (features.Contains(WGPUFeatureName_TextureFormatsTier2))
        {
        }
        if (features.Contains(WGPUFeatureName_Depth32FloatStencil8))
        {
            FeaturesPerFormat[(int32)PixelFormat::D32_Float_S8X24_UInt].Support |= supportsBuffer | supportsTexture | supportsDepth;
        }
        if (features.Contains(WGPUFeatureName_Float32Blendable))
        {
            FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support |= FormatSupport::Blendable;
            FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support |= FormatSupport::Blendable;
            FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support |= FormatSupport::Blendable;
        }
        if (features.Contains(WGPUFeatureName_Float32Filterable))
        {
            FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support |= FormatSupport::ShaderSample;
            FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support |= FormatSupport::ShaderSample;
            FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support |= FormatSupport::ShaderSample;
        }
        if (features.Contains(WGPUFeatureName_RG11B10UfloatRenderable))
        {
            FeaturesPerFormat[(int32)PixelFormat::R11G11B10_Float].Support |= supportsRender | supportsMSAA;
        }
        if (features.Contains(WGPUFeatureName_TextureCompressionBC))
        {
            auto supportBc =
                    FormatSupport::Texture2D |
                    FormatSupport::ShaderLoad |
                    FormatSupport::ShaderSample |
                    FormatSupport::Mip;
            if (features.Contains(WGPUFeatureName_TextureCompressionBCSliced3D))
                supportBc |= FormatSupport::Texture3D;
            FeaturesPerFormat[(int32)PixelFormat::BC1_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC1_UNorm_sRGB].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC2_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC2_UNorm_sRGB].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC3_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC3_UNorm_sRGB].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC4_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC4_SNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC5_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC5_SNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC6H_Uf16].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC6H_Sf16].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC7_UNorm].Support |= supportBc;
            FeaturesPerFormat[(int32)PixelFormat::BC7_UNorm_sRGB].Support |= supportBc;
        }
        if (features.Contains(WGPUFeatureName_TextureCompressionASTC))
        {
            auto supportAstc =
                    FormatSupport::Texture2D |
                    FormatSupport::ShaderLoad |
                    FormatSupport::ShaderSample |
                    FormatSupport::Mip;
            if (features.Contains(WGPUFeatureName_TextureCompressionASTCSliced3D))
                supportAstc |= FormatSupport::Texture3D;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_4x4_UNorm].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_4x4_UNorm_sRGB].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_6x6_UNorm].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_6x6_UNorm_sRGB].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_8x8_UNorm].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_8x8_UNorm_sRGB].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_10x10_UNorm].Support |= supportAstc;
            FeaturesPerFormat[(int32)PixelFormat::ASTC_10x10_UNorm_sRGB].Support |= supportAstc;
        }
        for (auto& e : FeaturesPerFormat)
        {
            if (EnumHasAllFlags(e.Support, FormatSupport::MultisampleRenderTarget))
                e.MSAALevelMax = MSAALevel::X4;
        }
#undef ADD_FORMAT

#if BUILD_DEBUG
        LOG(Info, "WebGPU limits:");
        LOG(Info, " > maxTextureDimension1D = {}", limits.maxTextureDimension1D);
        LOG(Info, " > maxTextureDimension2D = {}", limits.maxTextureDimension2D);
        LOG(Info, " > maxTextureDimension3D = {}", limits.maxTextureDimension3D);
        LOG(Info, " > maxTextureArrayLayers = {}", limits.maxTextureArrayLayers);
        LOG(Info, " > maxBindGroups = {}", limits.maxBindGroups);
        LOG(Info, " > maxBindGroupsPlusVertexBuffers = {}", limits.maxBindGroupsPlusVertexBuffers);
        LOG(Info, " > maxBindingsPerBindGroup = {}", limits.maxBindingsPerBindGroup);
        LOG(Info, " > maxDynamicUniformBuffersPerPipelineLayout = {}", limits.maxDynamicUniformBuffersPerPipelineLayout);
        LOG(Info, " > maxDynamicStorageBuffersPerPipelineLayout = {}", limits.maxDynamicStorageBuffersPerPipelineLayout);
        LOG(Info, " > maxSampledTexturesPerShaderStage = {}", limits.maxSampledTexturesPerShaderStage);
        LOG(Info, " > maxSamplersPerShaderStage = {}", limits.maxSamplersPerShaderStage);
        LOG(Info, " > maxStorageBuffersPerShaderStage = {}", limits.maxStorageBuffersPerShaderStage);
        LOG(Info, " > maxStorageTexturesPerShaderStage = {}", limits.maxStorageTexturesPerShaderStage);
        LOG(Info, " > maxUniformBuffersPerShaderStage = {}", limits.maxUniformBuffersPerShaderStage);
        LOG(Info, " > maxUniformBufferBindingSize = {}", limits.maxUniformBufferBindingSize);
        LOG(Info, " > maxStorageBufferBindingSize = {}", limits.maxStorageBufferBindingSize);
        LOG(Info, " > minUniformBufferOffsetAlignment = {}", limits.minUniformBufferOffsetAlignment);
        LOG(Info, " > minStorageBufferOffsetAlignment = {}", limits.minStorageBufferOffsetAlignment);
        LOG(Info, " > maxVertexBuffers = {}", limits.maxVertexBuffers);
        LOG(Info, " > maxBufferSize = {}", limits.maxBufferSize);
        LOG(Info, " > maxVertexAttributes = {}", limits.maxVertexAttributes);
        LOG(Info, " > maxVertexBufferArrayStride = {}", limits.maxVertexBufferArrayStride);
        LOG(Info, " > maxInterStageShaderVariables = {}", limits.maxInterStageShaderVariables);
        LOG(Info, " > maxColorAttachments = {}", limits.maxColorAttachments);
        LOG(Info, " > maxComputeWorkgroupStorageSize = {}", limits.maxComputeWorkgroupStorageSize);
        LOG(Info, " > maxComputeInvocationsPerWorkgroup = {}", limits.maxComputeInvocationsPerWorkgroup);
        LOG(Info, " > maxComputeWorkgroupSize = {}x{}x{}", limits.maxComputeWorkgroupSizeX, limits.maxComputeWorkgroupSizeY, limits.maxComputeWorkgroupSizeZ);
        LOG(Info, " > maxComputeWorkgroupsPerDimension = {}", limits.maxComputeWorkgroupsPerDimension);
        LOG(Info, " > maxImmediateSize = {}", limits.maxImmediateSize);
#endif
    }

    // Inti device options
    WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    deviceDesc.requiredLimits = &limits;
    deviceDesc.requiredFeatureCount = features.Count();
    deviceDesc.requiredFeatures = features.Get();
    deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        if (reason == WGPUDeviceLostReason_Destroyed || 
            reason == WGPUDeviceLostReason_FailedCreation || 
            reason == WGPUDeviceLostReason_CallbackCancelled)
            return;
        Platform::Fatal(String::Format(TEXT("WebGPU device lost! {}, 0x:{:x}"), WEBGPU_TO_STR(message), (uint32)reason), nullptr, FatalErrorType::GPUHang);
    };
    deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        if (type == WGPUErrorType_OutOfMemory)
        {
            Platform::Fatal(String::Format(TEXT("WebGPU device run out of memory! {}"), WEBGPU_TO_STR(message)), nullptr, FatalErrorType::GPUOutOfMemory);
        }
#if !BUILD_RELEASE
        else if (type == WGPUErrorType_Validation)
        {
            LOG(Warning, "WebGPU Validation: {}", WEBGPU_TO_STR(message));
        }
        else
        {
            LOG(Info, "WebGPU: {}", WEBGPU_TO_STR(message));
        }
        static int32 LogSpamLeft = 20;
        if (LogSpamLeft-- < 0)
            CRASH; // Too many errors
#endif
    };

    // Create device
    struct DeviceUserData : AsyncCallbackDataWebGPU
    {
        WGPUDevice Device = nullptr;
    };
    AsyncCallbackWebGPU<WGPURequestDeviceCallbackInfo, DeviceUserData> deviceRequest(WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT);
    deviceDesc.label = WEBGPU_STR("Flax Device");
    deviceDesc.defaultQueue.label = WEBGPU_STR("Flax Queue");
    deviceRequest.Info.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        DeviceUserData& userData = *reinterpret_cast<DeviceUserData*>(userdata1);
        userData.Device = device;
        userData.Call(status == WGPURequestDeviceStatus_Success, status, message);
    };
    wgpuAdapterRequestDevice(Adapter->Adapter, &deviceDesc, deviceRequest.Info);
    auto deviceRequestResult = deviceRequest.Wait();
    if (deviceRequestResult == WGPUWaitStatus_TimedOut)
    {
        LOG(Fatal, "WebGPU device request has timed out after {}s", deviceRequest.Data.WaitTime);
        return true;
    }
    if (deviceRequestResult == WGPUWaitStatus_Error)
    {
        LOG(Fatal, "Failed to get WebGPU device (browser might not support it). Error: {}, 0x{:x}", deviceRequest.Data.Message, deviceRequest.Data.Status);
        return true;
    }
    Device = deviceRequest.Data.Device;

    TotalGraphicsMemory = 1024 * 1024 * 1024; // Dummy 1GB

    // Create default samplers
    auto samplerDesc = GPUSamplerDescription::New();
#define INIT_SAMPLER(slot, filter, addressMode, compare) \
    DefaultSamplers[slot] = New<GPUSamplerWebGPU>(this); \
    samplerDesc.Filter = filter; \
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = addressMode; \
    samplerDesc.ComparisonFunction = compare; \
    DefaultSamplers[slot]->Init(samplerDesc)
    INIT_SAMPLER(0, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
    INIT_SAMPLER(1, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
    INIT_SAMPLER(2, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
    INIT_SAMPLER(3, GPUSamplerFilter::Point, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
    INIT_SAMPLER(4, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
    INIT_SAMPLER(5, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
#undef INIT_SAMPLER

    // Setup commands processing
    DataUploader._device = Device;
    Queue = wgpuDeviceGetQueue(Device);
    _mainContext = New<GPUContextWebGPU>(this);

    _state = DeviceState::Ready;
    return GPUDevice::Init();
}

void GPUDeviceWebGPU::DrawBegin()
{
    GPUDevice::DrawBegin();

    DataUploader.DrawBegin();

    // Create default texture
    static_assert(ARRAY_COUNT(DefaultTexture) == (int32)SpirvShaderResourceType::MAX, "Invalid size of default textures");
    if (!DefaultTexture[(int32)SpirvShaderResourceType::Texture2D])
    {
        GPUTextureWebGPU* texture;
#define INIT_TEXTURE(type, desc) \
    texture = New<GPUTextureWebGPU>(this, TEXT("DefaultTexture")); \
    DefaultTexture[(int32)SpirvShaderResourceType::type] = texture; \
    texture->Init(desc); \
    texture->SetResidentMipLevels(1)
        INIT_TEXTURE(Texture2D, GPUTextureDescription::New2D(1, 1, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::ShaderResource));
        INIT_TEXTURE(Texture3D, GPUTextureDescription::New3D(1, 1, 1, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::ShaderResource));
        INIT_TEXTURE(TextureCube, GPUTextureDescription::NewCube(1, 1, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::ShaderResource));
#undef INIT_TEXTURE
        
    }
}

GPUDeviceWebGPU::~GPUDeviceWebGPU()
{
    // Ensure to be disposed
    GPUDeviceWebGPU::Dispose();
}

GPUDevice* CreateGPUDeviceWebGPU()
{
    // Create instance
    WGPUInstanceDescriptor instanceDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        LOG(Error, "Failed to create instance for WebGPU (browser might not support it).");
        return nullptr;
    }

    // Init adapter options
    WGPURequestAdapterOptions adapterOptions = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
#if !PLATFORM_WEB
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
#endif

    // Create adapter
    struct AdapterUserData : AsyncCallbackDataWebGPU
    {
        WGPUAdapter Adapter = nullptr;
    };
    AsyncCallbackWebGPU<WGPURequestAdapterCallbackInfo, AdapterUserData> adapterRequest(WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT);
    adapterRequest.Info.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        AdapterUserData& userData = *reinterpret_cast<AdapterUserData*>(userdata1);
        userData.Adapter = adapter;
        userData.Call(status == WGPURequestAdapterStatus_Success, status, message);
    };
    wgpuInstanceRequestAdapter(instance, &adapterOptions, adapterRequest.Info);
    auto adapterRequestResult = adapterRequest.Wait();
    if (adapterRequestResult == WGPUWaitStatus_TimedOut)
    {
        LOG(Fatal, "WebGPU adapter request has timed out after {}s", adapterRequest.Data.WaitTime);
        return nullptr;
    }
    if (adapterRequestResult == WGPUWaitStatus_Error)
    {
        LOG(Fatal, "Failed to get WebGPU adapter (browser might not support it). Error: {}, 0x{:x}", adapterRequest.Data.Message, adapterRequest.Data.Status);
        return nullptr;
    }

    // Create device
    auto device = New<GPUDeviceWebGPU>(instance, New<GPUAdapterWebGPU>(adapterRequest.Data.Adapter));
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

void GPUDeviceWebGPU::Dispose()
{
    GPUDeviceLock lock(this);
    if (_state == DeviceState::Disposed)
        return;

    // Begin destruction
    _state = DeviceState::Disposing;
    WaitForGPU();
    preDispose();

    // Clear device resources
    DataUploader.ReleaseGPU();
    SAFE_DELETE_GPU_RESOURCES(DefaultTexture);
    SAFE_DELETE_GPU_RESOURCES(DefaultSamplers);
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(Adapter);
    if (Queue)
    {
        wgpuQueueRelease(Queue);
        Queue = nullptr;
    }
    if (Device)
    {
        wgpuDeviceRelease(Device);
        Device = nullptr;
    }
    wgpuInstanceRelease(WebGPUInstance);

    // End destruction
    GPUDevice::Dispose();
    _state = DeviceState::Disposed;
}

void GPUDeviceWebGPU::WaitForGPU()
{
}

bool GPUDeviceWebGPU::GetQueryResult(uint64 queryID, uint64& result, bool wait)
{
    // TODO: impl queries
    return false;
}

GPUTexture* GPUDeviceWebGPU::CreateTexture(const StringView& name)
{
    PROFILE_MEM(GraphicsTextures);
    return New<GPUTextureWebGPU>(this, name);
}

GPUShader* GPUDeviceWebGPU::CreateShader(const StringView& name)
{
    PROFILE_MEM(GraphicsShaders);
    return New<GPUShaderWebGPU>(this, name);
}

GPUPipelineState* GPUDeviceWebGPU::CreatePipelineState()
{
    PROFILE_MEM(GraphicsCommands);
    return New<GPUPipelineStateWebGPU>(this);
}

GPUTimerQuery* GPUDeviceWebGPU::CreateTimerQuery()
{
    return nullptr;
}

GPUBuffer* GPUDeviceWebGPU::CreateBuffer(const StringView& name)
{
    PROFILE_MEM(GraphicsBuffers);
    return New<GPUBufferWebGPU>(this, name);
}

GPUSampler* GPUDeviceWebGPU::CreateSampler()
{
    return New<GPUSamplerWebGPU>(this);
}

GPUVertexLayout* GPUDeviceWebGPU::CreateVertexLayout(const VertexElements& elements, bool explicitOffsets)
{
    return New<GPUVertexLayoutWebGPU>(this, elements, explicitOffsets);
}

GPUSwapChain* GPUDeviceWebGPU::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainWebGPU>(this, window);
}

GPUConstantBuffer* GPUDeviceWebGPU::CreateConstantBuffer(uint32 size, const StringView& name)
{
    PROFILE_MEM(GraphicsShaders);
    return New<GPUConstantBufferWebGPU>(this, size, name);
}

#endif
