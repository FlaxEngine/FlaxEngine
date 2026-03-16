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

GPUQuerySetWebGPU::GPUQuerySetWebGPU(WGPUDevice device, GPUQueryType type, uint32 count)
    : _device(device)
    , _count(count)
    , Type(type)
{
    // Timer queries use 2 items for begin/end timestamps
    ASSERT_LOW_LAYER(count % 2 == 0 || type != GPUQueryType::Timer);
    if (type == GPUQueryType::Timer)
        _index = 2; // Skip first item in timer queries due to bug in makePassTimestampWrites that cannot pass undefined value properly

    // Create query set
    WGPUQuerySetDescriptor desc = WGPU_QUERY_SET_DESCRIPTOR_INIT;
    desc.type = type == GPUQueryType::Timer ? WGPUQueryType_Timestamp : WGPUQueryType_Occlusion;
    desc.count = count;
    Set = wgpuDeviceCreateQuerySet(device, &desc);
    ASSERT(Set);

    // Create buffer for queries data
    WGPUBufferDescriptor bufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    bufferDesc.size = count * sizeof(uint64);
    bufferDesc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
    _queryBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    ASSERT(_queryBuffer);

    // Create buffer for reading copied queries data on CPU
    bufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    _readBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    ASSERT(_readBuffer);

#if COMPILE_WITH_PROFILER
    _memorySize = bufferDesc.size * 3; // Set + QueryBuffer + ReadBuffer
    PROFILE_MEM_INC(GraphicsCommands, _memorySize);
#endif
}

GPUQuerySetWebGPU::~GPUQuerySetWebGPU()
{
    PROFILE_MEM_DEC(GraphicsCommands, _memorySize);
    wgpuBufferDestroy(_readBuffer);
    wgpuBufferRelease(_readBuffer);
    wgpuBufferDestroy(_queryBuffer);
    wgpuBufferRelease(_queryBuffer);
    wgpuQuerySetDestroy(Set);
    wgpuQuerySetRelease(Set);
}

bool GPUQuerySetWebGPU::CanAllocate() const
{
    return _index < _count && (_state == Active || _state == Mapped);
}

uint32 GPUQuerySetWebGPU::Allocate()
{
    if (_state == Mapped)
    {
        // Start a new batch from the beginning
        wgpuBufferUnmap(_readBuffer);
        _state = Active;
        _index = 2;
        _mapped = nullptr;
    }
    uint32 index = _index;
    _index += Type == GPUQueryType::Timer ? 2 : 1;
    return index;
}

void GPUQuerySetWebGPU::Resolve(WGPUCommandEncoder encoder)
{
    ASSERT(_index != 0 && _state == Active);
    wgpuCommandEncoderResolveQuerySet(encoder, Set, 0, _index, _queryBuffer, 0);
    wgpuCommandEncoderCopyBufferToBuffer(encoder, _queryBuffer, 0, _readBuffer, 0, _index * sizeof(uint64));
    _state = Resolved;
}

bool GPUQuerySetWebGPU::Read(uint32 index, uint64& result, bool wait)
{
    if (_state == Resolved)
    {
        // Start mapping the buffer
        ASSERT(!wait); // TODO: impl wgpuBufferMapAsync with waiting (see GPUBufferWebGPU::Map)
        WGPUBufferMapCallbackInfo callback = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        callback.mode = WGPUCallbackMode_AllowSpontaneous;
        callback.userdata1 = this;
        callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
        {
            if (status == WGPUMapAsyncStatus_Success)
            {
                auto set = (GPUQuerySetWebGPU*)userdata1;
                set->OnRead();
            }
#if !BUILD_RELEASE
            else
            {
                LOG(Error, "Query Set map failed with status {}, {}", (uint32)status, WEBGPU_TO_STR(message));
            }
#endif
        };
        wgpuBufferMapAsync(_readBuffer, WGPUMapMode_Read, 0, _index * sizeof(uint64), callback);
        _state = Mapping;
    }
    else if (_state == Mapped)
    {
        // Read the results from mapped buffer
        if (Type == GPUQueryType::Timer)
        {
            // Timestamp calculates a difference between two queries (begin/end) in nanoseconds (result is in microseconds)
            result = Math::Max(_mapped[index + 1] - _mapped[index], 0ull) / 1000;
        }
        else
        {
            // Occlusion outputs number of fragment samples that pass all the tests (scissor, stencil, depth, etc.)
            result = _mapped[index];
        }
        return true;
    }
    return false;
}

void GPUQuerySetWebGPU::OnRead()
{
    // Get mapped buffer pointer
    ASSERT(_state == Mapping);
    _state = Mapped;
    _mapped = (const uint64*)wgpuBufferGetConstMappedRange(_readBuffer, 0, _index * sizeof(uint64));
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
        TimestampQuery = features.Contains(WGPUFeatureName_TimestampQuery);
        Limits.HasCompute =
            limits.maxStorageBuffersPerShaderStage >= GPU_MAX_UA_BINDED &&
            limits.maxStorageTexturesPerShaderStage >= GPU_MAX_UA_BINDED &&
            limits.maxComputeWorkgroupsPerDimension >= GPU_MAX_CS_DISPATCH_THREAD_GROUPS &&
            limits.maxComputeWorkgroupSizeX >= 1024 &&
            limits.maxComputeWorkgroupSizeY >= 256 &&
            limits.maxComputeWorkgroupSizeZ >= 8 &&
            limits.maxBufferSize >= 64 * 1024 * 1024; // 64MB
        Limits.HasInstancing = true;
        Limits.HasDrawIndirect = true;
        Limits.HasDepthAsSRV = true;
        Limits.HasReadOnlyDepth = true;
        Limits.HasDepthClip = features.Contains(WGPUFeatureName_DepthClipControl);
        Limits.HasReadOnlyDepth = true;
        Limits.MaximumSamplerAnisotropy = 4;
        Limits.MaximumTexture1DSize = limits.maxTextureDimension1D;
        Limits.MaximumTexture2DSize = limits.maxTextureDimension2D;
        Limits.MaximumTexture3DSize = limits.maxTextureDimension3D;
        Limits.MaximumMipLevelsCount = (int32)log2(limits.maxTextureDimension2D);
        Limits.MaximumTexture1DArraySize = Limits.MaximumTexture2DArraySize = limits.maxTextureArrayLayers;
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
        auto supportsBasicStorage = FormatSupport::UnorderedAccessReadOnly | FormatSupport::UnorderedAccessWriteOnly;

#define ADD_FORMAT(pixelFormat, support)
        FeaturesPerFormat[(int32)PixelFormat::R8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8_UInt].Support |= supportsBuffer | supportsTexture | supportsRender;
        FeaturesPerFormat[(int32)PixelFormat::R8_SInt].Support |= supportsBuffer | supportsTexture | supportsRender;
        FeaturesPerFormat[(int32)PixelFormat::R16_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R16_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R16_Float].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_UNorm].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_SNorm].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R8G8_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget | FormatSupport::UnorderedAccess;
        FeaturesPerFormat[(int32)PixelFormat::R32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget | FormatSupport::UnorderedAccess;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_SInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R16G16_Float].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UNorm_sRGB].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SNorm].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UInt].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SInt].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::B8G8R8A8_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UInt].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA;
        FeaturesPerFormat[(int32)PixelFormat::R11G11B10_Float].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R9G9B9E5_SharedExp].Support |= supportsBuffer | supportsTexture;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UInt].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SInt].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_Float].Support |= supportsBuffer | supportsTexture | supportsRender | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support & ~FormatSupport::ShaderSample;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_UInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget | supportsBasicStorage;
        FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_SInt].Support |= supportsBuffer | supportsTexture | FormatSupport::RenderTarget | supportsBasicStorage;
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
            FeaturesPerFormat[(int32)PixelFormat::R32G32_Float].Support |= supportsMultisampling;
            FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UInt].Support |= supportsMultisampling;
        }
        if (features.Contains(WGPUFeatureName_TextureFormatsTier1))
        {
            FeaturesPerFormat[(int32)PixelFormat::R8_UNorm].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R8_SNorm].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R8_UInt].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R8_SInt].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMSAA | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16_SNorm].Support |= supportsBuffer | supportsTexture | supportsBasicStorage;
            //FeaturesPerFormat[(int32)PixelFormat::R16_UInt].Support |= supportsBasicStorage; // TODO: fix issues with particle indices buffer that could use it
            FeaturesPerFormat[(int32)PixelFormat::R16_SInt].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16_Float].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_SNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_UInt].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_SInt].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16_Float].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SNorm].Support |= supportsBuffer | supportsTexture | supportsRender | supportsMultisampling | supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R10G10B10A2_UNorm].Support |= supportsBasicStorage;
            FeaturesPerFormat[(int32)PixelFormat::R11G11B10_Float].Support |= supportsBasicStorage;
        }
        if (features.Contains(WGPUFeatureName_TextureFormatsTier2))
        {
            FeaturesPerFormat[(int32)PixelFormat::R8_UNorm].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R8_UInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R8_SInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UNorm].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_UInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R8G8B8A8_SInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16_UInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16_SInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16_Float].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_UInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_SInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R16G16B16A16_Float].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_UInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_SInt].Support |= FormatSupport::UnorderedAccessReadWrite;
            FeaturesPerFormat[(int32)PixelFormat::R32G32B32A32_Float].Support |= FormatSupport::UnorderedAccessReadWrite;
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
                    FormatSupport::Texture1D |
                    FormatSupport::Texture2D |
                    FormatSupport::TextureCube |
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
                    FormatSupport::Texture1D |
                    FormatSupport::Texture2D |
                    FormatSupport::TextureCube |
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

    // Validate engine requirements from adapter limits
#define CHECK_LIMIT(limit, minValue) if (limits.limit < minValue) LOG(Fatal, "WebGPU adapter doesn't meet minimum requirements (unsupported device). {} limit is {} but needs to be {} (or higher).", TEXT(#limit), limits.limit, minValue)
    CHECK_LIMIT(maxBindGroups, GPUBindGroupsWebGPU::Pixel + 1); // 2 groups (one for Vertex Shader, other for Pixel Shader)
    CHECK_LIMIT(maxStorageBuffersPerShaderStage, 3); // ObjectsBuffer + BoneMatrices + PrevBoneMatrices (even more might be needed by game shaders)
    CHECK_LIMIT(maxSamplersPerShaderStage, 8); // Samplers
    CHECK_LIMIT(maxSampledTexturesPerShaderStage, 8); // Textures
    CHECK_LIMIT(maxDynamicUniformBuffersPerPipelineLayout, GPU_MAX_CB_BINDED); // Constant Buffers
    CHECK_LIMIT(maxColorAttachments, GPU_MAX_RT_BINDED); // Render Targets
    CHECK_LIMIT(maxVertexBuffers, GPU_MAX_VB_BINDED); // Vertex Buffers
    CHECK_LIMIT(maxVertexAttributes, 8); // Vertex Elements
    CHECK_LIMIT(maxBufferSize, 32ull * 1025 * 1024); // Min 32MB buffer
#undef CHECK_LIMIT

    // Init device options
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
            LOG(Fatal, "Too many WebGPU errors");
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

    // Create default resources
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
    {
        WGPUBufferDescriptor bufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
        bufferDesc.label = WEBGPU_STR("DefaultBuffer");
#endif
        bufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_Vertex;
        bufferDesc.size = 64;
        DefaultBuffer = wgpuDeviceCreateBuffer(Device, &bufferDesc);
    }

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
    for (int32 i = 0; i < QuerySetsCount; i++)
        Delete(QuerySets[i]);
    QuerySetsCount = 0;
    DataUploader.ReleaseGPU();
    SAFE_DELETE_GPU_RESOURCE(DefaultRenderTarget);
    SAFE_DELETE_GPU_RESOURCES(DefaultTexture);
    SAFE_DELETE_GPU_RESOURCES(DefaultSamplers);
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(Adapter);
    if (DefaultBuffer)
    {
        wgpuBufferRelease(DefaultBuffer);
        DefaultBuffer = nullptr;
    }
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
    if (QueueSubmits == 0)
        return;
    QueueSubmits = 0;
    AsyncCallbackWebGPU<WGPUQueueWorkDoneCallbackInfo> workDone(WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT);
    workDone.Info.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        AsyncCallbackDataWebGPU& userData = *reinterpret_cast<AsyncCallbackDataWebGPU*>(userdata1);
        userData.Call(status == WGPUQueueWorkDoneStatus_Success, status, message);
    };
    wgpuQueueOnSubmittedWorkDone(Queue, workDone.Info);
    auto workDoneResult = workDone.Wait();
    if (workDoneResult == WGPUWaitStatus_TimedOut)
    {
        LOG(Error, "WebGPU queue wait has timed out after {}s", workDone.Data.WaitTime);
        return;
    }
    if (workDoneResult == WGPUWaitStatus_Error)
    {
        LOG(Error, "Failed to wait for WebGPU queue. Error: {}, 0x{:x}", workDone.Data.Message, workDone.Data.Status);
        return;
    }
}

GPUQueryWebGPU GPUDeviceWebGPU::AllocateQuery(GPUQueryType type)
{
    // Ignore if device doesn't support timer queries
    if (type == GPUQueryType::Timer && !TimestampQuery)
        return {};

    // Get query set with free space
    int32 setIndex = 0;
    for (; setIndex < QuerySetsCount; setIndex++)
    {
        auto heap = QuerySets[setIndex];
        if (heap->Type == type && heap->CanAllocate())
            break;
    }
    if (setIndex == QuerySetsCount)
    {
        if (setIndex == WEBGPU_MAX_QUERY_SETS)
        {
#if !BUILD_RELEASE
            static bool SingleTimeLog = true;
            if (SingleTimeLog)
            {
                SingleTimeLog = false;
                LOG(Error, "Run out of the query sets capacity.");
            }
#endif
            return {};
        }

        // Allocate a new query heap
        PROFILE_MEM(GraphicsCommands);
        uint32 size = type == GPUQueryType::Occlusion ? 4096 : 1024;
        auto set = New<GPUQuerySetWebGPU>(Device, type, size);
        QuerySets[QuerySetsCount++] = set;
    }

    // Allocate query from the set
    GPUQueryWebGPU query;
    {
        static_assert(sizeof(GPUQueryWebGPU) == sizeof(uint64), "Invalid WebGPU query size.");
        query.Set = setIndex;
        query.Index = QuerySets[setIndex]->Allocate();
    }
    return query;
}

bool GPUDeviceWebGPU::GetQueryResult(uint64 queryID, uint64& result, bool wait)
{
    if (queryID == 0)
    {
        // Invalid query
        result = 0;
        return true;
    }

    GPUQueryWebGPU query;
    query.Raw = queryID;
    auto set = QuerySets[query.Set];
    return set->Read(query.Index, result, wait);
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
