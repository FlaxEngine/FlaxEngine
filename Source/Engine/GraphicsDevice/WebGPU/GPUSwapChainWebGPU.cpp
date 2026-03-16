// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUSwapChainWebGPU.h"
#include "GPUAdapterWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Platform/Window.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

GPUSwapChainWebGPU::GPUSwapChainWebGPU(GPUDeviceWebGPU* device, Window* window)
    : GPUResourceWebGPU(device, StringView::Empty)
{
    _window = window;
}

void GPUSwapChainWebGPU::OnReleaseGPU()
{
    // Release data
    PROFILE_MEM_DEC(Graphics, _memoryUsage);
    auto surfaceTexture = _surfaceView.Texture;
    _surfaceView.Release();
    if (surfaceTexture)
    {
        wgpuTextureRelease(surfaceTexture);
    }
    if (Surface)
    {
        wgpuSurfaceRelease(Surface);
        Surface = nullptr;
    }
    _width = _height = 0;
    _memoryUsage = 0;
}

bool GPUSwapChainWebGPU::IsFullscreen()
{
    return _window->IsFullscreen();
}

void GPUSwapChainWebGPU::SetFullscreen(bool isFullscreen)
{
    _window->SetIsFullscreen(true);
}

GPUTextureView* GPUSwapChainWebGPU::GetBackBufferView()
{
    if (!_surfaceView.Texture)
    {
        // Get current texture for the surface
        WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
        wgpuSurfaceGetCurrentTexture(Surface, &surfaceTexture);
        bool hasSurfaceTexture = surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal;
        ASSERT(hasSurfaceTexture);
        _surfaceView.Texture = surfaceTexture.texture;

        // Fix up the size (in case underlying texture is different than the engine resize it to)
        const uint32 width = wgpuTextureGetWidth(surfaceTexture.texture);
        const uint32 height = wgpuTextureGetHeight(surfaceTexture.texture);
        if (_width != width || _height != height)
        {
            PROFILE_MEM_DEC(Graphics, _memoryUsage);
            _width = width;
            _height = height;
            _memoryUsage = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1);
            PROFILE_MEM_INC(Graphics, _memoryUsage);
        }

        // Create view
        WGPUTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
        viewDesc.label = WEBGPU_STR("Flax Surface");
#endif
        viewDesc.format = wgpuTextureGetFormat(surfaceTexture.texture);
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;
        viewDesc.usage = wgpuTextureGetUsage(surfaceTexture.texture);
        _surfaceView.Create(surfaceTexture.texture, viewDesc);
    }
    return &_surfaceView;
}

void GPUSwapChainWebGPU::Present(bool vsync)
{
    PROFILE_CPU();
    ZoneColor(TracyWaitZoneColor);

#if !PLATFORM_WEB
    // Present frame
    wgpuSurfacePresent(Surface);
#endif

    // Release the texture
    auto surfaceTexture = _surfaceView.Texture;
    if (surfaceTexture)
    {
        _surfaceView.Release();
        wgpuTextureRelease(surfaceTexture);
    }

    // Base
    GPUSwapChain::Present(vsync);
}

bool GPUSwapChainWebGPU::Resize(int32 width, int32 height)
{
    if (width == _width && height == _height)
        return false;
    _device->WaitForGPU();
    GPUDeviceLock lock(_device);
#if GPU_ENABLE_DIAGNOSTICS
    LOG(Info, "Resizing WebGPU surface to: {}x{}", width, height);
#endif

    // Ensure to have a surface
    if (!Surface)
    {
        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
        canvasDesc.selector = WEBGPU_STR(WEB_CANVAS_ID);
        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.nextInChain = &canvasDesc.chain;
#if GPU_ENABLE_RESOURCE_NAMING
        surfaceDesc.label = WEBGPU_STR("Flax Surface");
#endif
        Surface = wgpuInstanceCreateSurface(_device->WebGPUInstance, &surfaceDesc);
        if (!Surface)
        {
            LOG(Fatal, "Failed to create WebGPU surface for the HTML selector '{}'", TEXT(WEB_CANVAS_ID));
            return true;
        }
    }

    // Setup surface configuration (based on capabilities)
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(Surface, _device->Adapter->Adapter, &capabilities);
    auto formats = ToSpan(capabilities.formats, capabilities.formatCount);
    auto presentModes = ToSpan(capabilities.presentModes, capabilities.presentModeCount);
    auto alphaModes = ToSpan(capabilities.alphaModes, capabilities.alphaModeCount);
    WGPUSurfaceConfiguration configuration = WGPU_SURFACE_CONFIGURATION_INIT;
    configuration.device = _device->Device;
    configuration.usage = WGPUTextureUsage_RenderAttachment;
#if GPU_USE_WINDOW_SRV
    configuration.usage |= WGPUTextureUsage_TextureBinding;
#endif
    configuration.width = width;
    configuration.height = height;
    configuration.format = WGPUTextureFormat_RGBA8Unorm;
    configuration.viewFormats = &configuration.format;
    configuration.viewFormatCount = 1;
    _format = PixelFormat::R8G8B8A8_UNorm;
    if (SpanContains(formats, RenderToolsWebGPU::ToTextureFormat(GPU_BACK_BUFFER_PIXEL_FORMAT)))
    {
        _format = GPU_BACK_BUFFER_PIXEL_FORMAT;
        configuration.format = RenderToolsWebGPU::ToTextureFormat(_format);
    }
    else if (formats.Length() != 0)
    {
        configuration.format = formats[0];
        _format = RenderToolsWebGPU::ToPixelFormat(configuration.format);
    }
    if (_window->GetSettings().SupportsTransparency && SpanContains(alphaModes, WGPUCompositeAlphaMode_Premultiplied))
        configuration.alphaMode = WGPUCompositeAlphaMode_Premultiplied;
    else if (SpanContains(alphaModes, WGPUCompositeAlphaMode_Opaque))
        configuration.alphaMode = WGPUCompositeAlphaMode_Opaque;
    if (SpanContains(presentModes, WGPUPresentMode_Mailbox))
        configuration.presentMode = WGPUPresentMode_Mailbox;
    if (SpanContains(presentModes, WGPUPresentMode_Fifo))
        configuration.presentMode = WGPUPresentMode_Fifo;
    else if (presentModes.Length() != 0)
        configuration.presentMode = presentModes[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    // Configure surface
    wgpuSurfaceConfigure(Surface, &configuration);

    // Init
    _surfaceView.Init(this, _format, MSAALevel::None);
    _width = width;
    _height = height;
    _memoryUsage = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1);
    PROFILE_MEM_INC(Graphics, _memoryUsage);

    return false;
}

#endif
