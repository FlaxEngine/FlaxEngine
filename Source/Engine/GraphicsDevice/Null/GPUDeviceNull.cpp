// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_NULL

#include "GPUDeviceNull.h"
#include "GPUContextNull.h"
#include "GPUAdapterNull.h"
#include "GPUTextureNull.h"
#include "GPUShaderNull.h"
#include "GPUPipelineStateNull.h"
#include "GPUTimerQueryNull.h"
#include "GPUBufferNull.h"
#include "GPUSamplerNull.h"
#include "GPUVertexLayoutNull.h"
#include "GPUSwapChainNull.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Graphics/Async/GPUTasksManager.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

namespace
{
    // The Null device creates stub resources and renders nothing, so any answer here is "safe" in the sense that no
    // hardware is touched. The answer still matters: GPUTexture::Init validates descriptions against it, and code
    // picks formats by probing it. Reporting nothing refuses every texture (code that creates textures cannot run
    // headless at all); reporting everything accepts descriptions no GPU would (a compressed render target, a depth
    // texture with unordered access) and hides fallback paths. A realistic table keeps headless behavior close to
    // what hardware does.
    FormatSupport NullFormatSupport(const PixelFormat format)
    {
        constexpr FormatSupport textures = FormatSupport::Texture1D | FormatSupport::Texture2D | FormatSupport::Texture3D | FormatSupport::TextureCube;
        constexpr FormatSupport sampled = textures | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::Mip | FormatSupport::CpuLockable;
        constexpr FormatSupport storage = FormatSupport::UnorderedAccess | FormatSupport::TypedUnorderedAccessView;
        constexpr FormatSupport buffers = FormatSupport::Buffer | FormatSupport::VertexBuffer;
        constexpr FormatSupport display = FormatSupport::Display | FormatSupport::BackBufferCast;

        switch (format)
        {
        case PixelFormat::Unknown:
        case PixelFormat::Basis: // Transcoded on the CPU into another format before any texture is created
        case PixelFormat::R1_UNorm: // Not supported by current desktop GPUs
            return FormatSupport::None;

        // Depth-stencil: attachments only, sampled through the typeless formats and their views below
        case PixelFormat::D32_Float_S8X24_UInt:
        case PixelFormat::D32_Float:
        case PixelFormat::D24_UNorm_S8_UInt:
        case PixelFormat::D16_UNorm:
            return FormatSupport::Texture2D | FormatSupport::TextureCube | FormatSupport::Mip | FormatSupport::DepthStencil;
        case PixelFormat::R32G8X24_Typeless:
        case PixelFormat::R24G8_Typeless:
            return FormatSupport::Texture2D | FormatSupport::TextureCube | FormatSupport::Mip | FormatSupport::CastWithinBitLayout;
        case PixelFormat::R32_Float_X8X24_Typeless:
        case PixelFormat::R24_UNorm_X8_Typeless:
            return FormatSupport::Texture2D | FormatSupport::TextureCube | FormatSupport::Mip | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::ShaderSampleComparison;
        case PixelFormat::X32_Typeless_G8X24_UInt:
        case PixelFormat::X24_Typeless_G8_UInt:
            return FormatSupport::Texture2D | FormatSupport::TextureCube | FormatSupport::Mip | FormatSupport::ShaderLoad;

        // Video: decoded frames, sampled and written by the decoder
        case PixelFormat::NV12:
            return FormatSupport::Texture2D | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::RenderTarget | FormatSupport::CpuLockable | FormatSupport::DecoderOutput | FormatSupport::VideoProcessorInput | FormatSupport::VideoProcessorOutput;
        case PixelFormat::YUY2:
            return FormatSupport::Texture2D | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::CpuLockable | FormatSupport::VideoProcessorInput;

        // Formats that can be sampled but not rendered into
        case PixelFormat::R9G9B9E5_SharedExp:
        case PixelFormat::R8G8_B8G8_UNorm:
        case PixelFormat::G8R8_G8B8_UNorm:
            return sampled;
        case PixelFormat::R32G32B32_Float:
            return sampled | buffers;
        case PixelFormat::R32G32B32_UInt:
        case PixelFormat::R32G32B32_SInt:
            return textures | FormatSupport::ShaderLoad | FormatSupport::Mip | FormatSupport::CpuLockable | buffers;

        // Presentable formats
        case PixelFormat::R10G10B10_Xr_Bias_A2_UNorm:
            return FormatSupport::Texture2D | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::RenderTarget | display;
        default:
            break;
        }

        if (PixelFormatExtensions::IsCompressedASTC(format))
            return FormatSupport::None; // Mobile only
        if (PixelFormatExtensions::IsTypeless(format, false))
            return textures | FormatSupport::Mip | FormatSupport::CastWithinBitLayout | (PixelFormatExtensions::IsCompressed(format) ? FormatSupport::None : FormatSupport::Buffer);
        if (PixelFormatExtensions::IsCompressedBC(format))
            return FormatSupport::Texture2D | FormatSupport::Texture3D | FormatSupport::TextureCube | FormatSupport::ShaderLoad | FormatSupport::ShaderSample | FormatSupport::Mip | FormatSupport::CpuLockable;

        if (PixelFormatExtensions::IsInteger(format))
        {
            // Loaded, not filtered or blended; the only formats usable as index buffers
            FormatSupport support = textures | FormatSupport::ShaderLoad | FormatSupport::Mip | FormatSupport::CpuLockable | FormatSupport::RenderTarget | buffers | storage;
            if (format == PixelFormat::R16_UInt || format == PixelFormat::R32_UInt)
                support |= FormatSupport::IndexBuffer;
            return support;
        }

        // Normalized and floating-point color formats
        FormatSupport support = sampled | buffers | FormatSupport::RenderTarget | FormatSupport::Blendable | FormatSupport::MultisampleResolve | FormatSupport::ShaderGather;
        if (!PixelFormatExtensions::IsSRGB(format))
            support |= storage; // No unordered access to sRGB textures
        if (format != PixelFormat::R8G8B8A8_SNorm && format != PixelFormat::R16G16B16A16_SNorm && format != PixelFormat::R16G16_SNorm && format != PixelFormat::R8G8_SNorm && format != PixelFormat::R16_SNorm && format != PixelFormat::R8_SNorm)
            support |= FormatSupport::MipAutogen;
        if (format == PixelFormat::R32_Float || format == PixelFormat::R16_UNorm)
            support |= FormatSupport::ShaderSampleComparison;
        switch (format)
        {
        case PixelFormat::R8G8B8A8_UNorm:
        case PixelFormat::R8G8B8A8_UNorm_sRGB:
        case PixelFormat::B8G8R8A8_UNorm:
        case PixelFormat::B8G8R8A8_UNorm_sRGB:
        case PixelFormat::R10G10B10A2_UNorm:
        case PixelFormat::R16G16B16A16_Float:
            support |= display;
            break;
        default:
            break;
        }
        return support;
    }
}


GPUDeviceNull::GPUDeviceNull()
    : GPUDevice(RendererType::Null, ShaderProfile::Unknown)
    , _mainContext(nullptr)
    , _adapter(New<GPUAdapterNull>())
{
}

GPUDevice* GPUDeviceNull::Create()
{
    // Create device
    auto device = New<GPUDeviceNull>();
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

GPUDeviceNull::~GPUDeviceNull()
{
    // Ensure to be disposed
    GPUDeviceNull::Dispose();
}

bool GPUDeviceNull::Init()
{
    TotalGraphicsMemory = 0;
    _state = DeviceState::Created;

    // Init device limits
    {
        auto& limits = Limits;
        Platform::MemoryClear(&limits, sizeof(limits));
        limits.MaximumMipLevelsCount = 14;
        limits.MaximumTexture1DSize = 8192;
        limits.MaximumTexture1DArraySize = 512;
        limits.MaximumTexture2DSize = 8192;
        limits.MaximumTexture2DArraySize = 512;
        limits.MaximumTexture3DSize = 2048;
        limits.MaximumTextureCubeSize = 16384;
        limits.MaximumSamplerAnisotropy = 1;
        // Report what a typical desktop GPU would allow for each format (see NullFormatSupport), so that resource
        // creation behaves as it would on hardware: valid textures are created (as stubs), invalid ones are
        // refused with the same warnings. MSAA is not offered, since nothing is ever rendered.
        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
            FeaturesPerFormat[i] = FormatFeatures(MSAALevel::None, NullFormatSupport((PixelFormat)i));
    }

    // Create main context
    _mainContext = New<GPUContextNull>(this);

    _state = DeviceState::Ready;
    return GPUDevice::Init();
}

void GPUDeviceNull::Draw()
{
    DrawBegin();

    auto context = GetMainContext();

    RenderBegin();
    GetTasksManager()->FrameBegin();
    context->FrameBegin();

    // don't render anything

    context->FrameEnd();
    GetTasksManager()->FrameEnd();
    RenderEnd();

    DrawEnd();
}

void GPUDeviceNull::Dispose()
{
    GPUDeviceLock lock(this);

    // Check if has been disposed already
    if (_state == DeviceState::Disposed)
        return;

    // Set current state
    _state = DeviceState::Disposing;

    // Wait for rendering end
    WaitForGPU();

    // Pre dispose
    preDispose();

    // Clear stuff
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(_adapter);

    // Base
    GPUDevice::Dispose();

    // Set current state
    _state = DeviceState::Disposed;
}

GPUContext* GPUDeviceNull::GetMainContext()
{
    return reinterpret_cast<GPUContext*>(_mainContext);
}

GPUAdapter* GPUDeviceNull::GetAdapter() const
{
    return reinterpret_cast<GPUAdapter*>(_adapter);
}

void* GPUDeviceNull::GetNativePtr() const
{
    return nullptr;
}

bool GPUDeviceNull::LoadContent()
{
    // Skip loading resources
    return false;
}

void GPUDeviceNull::WaitForGPU()
{
}

bool GPUDeviceNull::GetQueryResult(uint64 queryID, uint64& result, bool wait)
{
    return false;
}

GPUTexture* GPUDeviceNull::CreateTexture(const StringView& name)
{
    PROFILE_MEM(GraphicsTextures);
    return New<GPUTextureNull>();
}

GPUShader* GPUDeviceNull::CreateShader(const StringView& name)
{
    PROFILE_MEM(GraphicsShaders);
    return New<GPUShaderNull>();
}

GPUPipelineState* GPUDeviceNull::CreatePipelineState()
{
    PROFILE_MEM(GraphicsCommands);
    return New<GPUPipelineStateNull>();
}

GPUTimerQuery* GPUDeviceNull::CreateTimerQuery()
{
    return New<GPUTimerQueryNull>();
}

GPUBuffer* GPUDeviceNull::CreateBuffer(const StringView& name)
{
    PROFILE_MEM(GraphicsBuffers);
    return New<GPUBufferNull>();
}

GPUSampler* GPUDeviceNull::CreateSampler()
{
    return New<GPUSamplerNull>();
}

GPUVertexLayout* GPUDeviceNull::CreateVertexLayout(const VertexElements& elements, bool explicitOffsets)
{
    return New<GPUVertexLayoutNull>(elements);
}

GPUSwapChain* GPUDeviceNull::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainNull>(window);
}

GPUConstantBuffer* GPUDeviceNull::CreateConstantBuffer(uint32 size, const StringView& name)
{
    return nullptr;
}

GPUDevice* CreateGPUDeviceNull()
{
    return GPUDeviceNull::Create();
}

#endif
