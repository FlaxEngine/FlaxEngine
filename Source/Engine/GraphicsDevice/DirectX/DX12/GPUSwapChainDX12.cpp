// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUSwapChainDX12.h"
#include "GPUContextDX12.h"
#include "../IncludeDirectXHeaders.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

void BackBufferDX12::Setup(GPUSwapChainDX12* window, ID3D12Resource* backbuffer)
{
    // Cache handle and set default initial state for the backbuffers
    initResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT, 1);

    Handle.Init(window, window->GetDevice(), this, GPU_BACK_BUFFER_PIXEL_FORMAT, MSAALevel::None);

    // Create RTV
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtDesc;
        rtDesc.Format = (DXGI_FORMAT)GPU_BACK_BUFFER_PIXEL_FORMAT;
        rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtDesc.Texture2D.MipSlice = 0;
        rtDesc.Texture2D.PlaneSlice = 0;
        Handle.SetRTV(rtDesc);
    }

#if GPU_USE_WINDOW_SRV
    // Create SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srDesc;
        srDesc.Format = (DXGI_FORMAT)GPU_BACK_BUFFER_PIXEL_FORMAT;
        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srDesc.Texture2D.MostDetailedMip = 0;
        srDesc.Texture2D.MipLevels = 1;
        srDesc.Texture2D.ResourceMinLODClamp = 0;
        srDesc.Texture2D.PlaneSlice = 0;
        Handle.SetSRV(srDesc);
    }
#endif
}

void BackBufferDX12::Release()
{
    Handle.Release();
    SAFE_RELEASE(_resource);
}

GPUSwapChainDX12::GPUSwapChainDX12(GPUDeviceDX12* device, Window* window)
    : GPUResourceDX12(device, StringView::Empty)
    , _windowHandle(static_cast<HWND>(window->GetNativePtr()))
    , _swapChain(nullptr)
    , _currentFrameIndex(0)
    , _allowTearing(false)
    , _isFullscreen(false)
{
    ASSERT(_windowHandle);
    _window = window;
}

void GPUSwapChainDX12::OnReleaseGPU()
{
    _device->WaitForGPU();

#if PLATFORM_WINDOWS
    // Disable fullscreen mode
    if (_swapChain)
    {
        VALIDATE_DIRECTX_CALL(_swapChain->SetFullscreenState(false, nullptr));
    }
#endif

    // Release data
    releaseBackBuffer();
    _backBuffers.Resize(0);
    if (_swapChain)
    {
        _device->AddResourceToLateRelease(_swapChain);
        _swapChain = nullptr;
    }
    _width = _height = 0;
}

void GPUSwapChainDX12::releaseBackBuffer()
{
    for (int32 i = 0; i < _backBuffers.Count(); i++)
    {
        _backBuffers[i].Release();
    }
}

bool GPUSwapChainDX12::IsFullscreen()
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    return true;
#else
    // Check if has no swap chain created
    if (_swapChain == nullptr)
        return false;

    // Get state
    BOOL state;
    VALIDATE_DIRECTX_CALL(_swapChain->GetFullscreenState(&state, nullptr));
    return state == TRUE;
#endif
}

void GPUSwapChainDX12::SetFullscreen(bool isFullscreen)
{
#if PLATFORM_WINDOWS
    if (_swapChain && isFullscreen != IsFullscreen())
    {
        _device->WaitForGPU();
        GPUDeviceLock lock(_device);

        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        _swapChain->GetDesc(&swapChainDesc);

        // Setup target for fullscreen mode
        IDXGIOutput* output = nullptr;
        if (isFullscreen && _device->Outputs.HasItems())
        {
            const uint32 outputIdx = 0;
            auto& outputDX = _device->Outputs[outputIdx];
            output = outputDX.Output.Get();
            swapChainDesc.BufferDesc = outputDX.DesktopViewMode;
        }

        if (FAILED(_swapChain->ResizeTarget(&swapChainDesc.BufferDesc)))
        {
            LOG(Warning, "Swapchain resize failed.");
        }

        if (FAILED(_swapChain->SetFullscreenState(isFullscreen, output)))
        {
            LOG(Warning, "Cannot change fullscreen mode for '{0}' to {1}.", ToString(), isFullscreen);
        }

        _isFullscreen = isFullscreen;

        // Buffers must be resized in flip presentation model
        if (swapChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ||
            swapChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
        {
            const int32 width = _width;
            const int32 height = _height;
            _width = _height = 0;
            Resize(width, height);
        }
    }
#else
    LOG(Info, "Cannot change fullscreen mode on this platform");
#endif
}

bool GPUSwapChainDX12::Resize(int32 width, int32 height)
{
    // Check if window state won't change
    if (width == _width && height == _height)
    {
        return false;
    }

    _device->WaitForGPU();
    GPUDeviceLock lock(_device);
    _allowTearing = _device->AllowTearing;
    _format = GPU_BACK_BUFFER_PIXEL_FORMAT;

#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    ReleaseGPU();

    _currentFrameIndex = 0;
    _width = width;
    _height = height;
    _memoryUsage = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * DX12_BACK_BUFFER_COUNT;

    getBackBuffer();
#else
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    if (_swapChain == nullptr)
    {
        ReleaseGPU();

        // Create swap chain description
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = RenderToolsDX::ToDxgiFormat(_format);
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = DX12_BACK_BUFFER_COUNT;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#if GPU_USE_WINDOW_SRV
        swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
#endif
        if (_allowTearing)
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
        if (_device->Outputs.HasItems())
        {
            const uint32 outputIdx = 0;
            auto& output = _device->Outputs[outputIdx];
            fullscreenDesc.RefreshRate = output.DesktopViewMode.RefreshRate;
            fullscreenDesc.Scaling = output.DesktopViewMode.Scaling;
            fullscreenDesc.ScanlineOrdering = output.DesktopViewMode.ScanlineOrdering;
            fullscreenDesc.Windowed = TRUE;
        }
        else
        {
            fullscreenDesc.RefreshRate.Numerator = 0;
            fullscreenDesc.RefreshRate.Denominator = 1;
            fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            fullscreenDesc.Windowed = TRUE;
        }

        // Create swap chain (it needs the queue so that it can force a flush on it)
        IDXGISwapChain1* swapChain;
        auto dxgiFactory = _device->GetDXGIFactory();
        VALIDATE_DIRECTX_CALL(dxgiFactory->CreateSwapChainForHwnd(_device->GetCommandQueueDX12(), _windowHandle, &swapChainDesc, &fullscreenDesc, nullptr, &swapChain));
        _swapChain = static_cast<IDXGISwapChain3*>(swapChain);
        ASSERT(_swapChain);
        DX_SET_DEBUG_NAME_EX(_swapChain, TEXT("RenderOutput"), TEXT("SwapChain"), TEXT(""));
        _swapChain->SetBackgroundColor((const DXGI_RGBA*)Color::Black.Raw);
        _backBuffers.Resize(swapChainDesc.BufferCount);

        // Disable DXGI changes to the window
        VALIDATE_DIRECTX_CALL(dxgiFactory->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER));
    }
    else
    {
        releaseBackBuffer();

        _swapChain->GetDesc1(&swapChainDesc);

        VALIDATE_DIRECTX_CALL(_swapChain->ResizeBuffers(swapChainDesc.BufferCount, width, height, swapChainDesc.Format, swapChainDesc.Flags));
    }

    _currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();
    _width = width;
    _height = height;
    _memoryUsage = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * swapChainDesc.BufferCount;

    getBackBuffer();
#endif

    return false;
}

void GPUSwapChainDX12::CopyBackbuffer(GPUContext* context, GPUTexture* dst)
{
    const auto contextDX12 = (GPUContextDX12*)context;

    auto dstDX12 = (GPUTextureDX12*)dst;
    auto backbuffer = &_backBuffers[_currentFrameIndex];

    contextDX12->SetResourceState(dstDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    contextDX12->SetResourceState(backbuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
    contextDX12->FlushResourceBarriers();

    if (dst->IsStaging())
    {
        const int32 copyOffset = dstDX12->ComputeBufferOffset(0, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        D3D12_TEXTURE_COPY_LOCATION dstLocation;
        dstLocation.pResource = dstDX12->GetResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLocation.PlacedFootprint.Offset = copyOffset;
        dstLocation.PlacedFootprint.Footprint.Width = dstDX12->CalculateMipSize(dstDX12->Width(), 0);
        dstLocation.PlacedFootprint.Footprint.Height = dstDX12->CalculateMipSize(dstDX12->Height(), 0);
        dstLocation.PlacedFootprint.Footprint.Depth = dstDX12->CalculateMipSize(dstDX12->Depth(), 0);
        dstLocation.PlacedFootprint.Footprint.Format = RenderToolsDX::ToDxgiFormat(dstDX12->Format());
        dstLocation.PlacedFootprint.Footprint.RowPitch = dstDX12->ComputeRowPitch(0, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

        D3D12_TEXTURE_COPY_LOCATION srcLocation;
        srcLocation.pResource = backbuffer->GetResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = 0;

        contextDX12->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }
    else
    {
        contextDX12->GetCommandList()->CopyResource(dstDX12->GetResource(), backbuffer->GetResource());
    }
}

void GPUSwapChainDX12::getBackBuffer()
{
    _backBuffers.Resize(DX12_BACK_BUFFER_COUNT);
    for (int32 i = 0; i < _backBuffers.Count(); i++)
    {
        ID3D12Resource* backbuffer;
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
        D3D12_HEAP_PROPERTIES swapChainHeapProperties;
        swapChainHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        swapChainHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        swapChainHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        swapChainHeapProperties.CreationNodeMask = 1;
        swapChainHeapProperties.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC swapChainBufferDesc;
        swapChainBufferDesc.MipLevels = 1;
        swapChainBufferDesc.Format = RenderToolsDX::ToDxgiFormat(_format);
        swapChainBufferDesc.Width = _width;
        swapChainBufferDesc.Height = _height;
        swapChainBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        swapChainBufferDesc.DepthOrArraySize = 1;
        swapChainBufferDesc.SampleDesc.Count = 1;
        swapChainBufferDesc.SampleDesc.Quality = 0;
        swapChainBufferDesc.Alignment = 0;
        swapChainBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        swapChainBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
        swapChainOptimizedClearValue.Format = swapChainBufferDesc.Format;
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(&backbuffer)));
#else
        VALIDATE_DIRECTX_CALL(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
#endif
        DX_SET_DEBUG_NAME_EX(backbuffer, TEXT("RenderOutput"), TEXT("BackBuffer"), i);
        _backBuffers[i].Setup(this, backbuffer);
    }
}

#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE

void GPUSwapChainDX12::Begin(RenderTask* task)
{
    // Wait until frame start is signaled
    _framePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE, nullptr, D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE, &_framePipelineToken));

    GPUSwapChain::Begin(task);
}

#endif

void GPUSwapChainDX12::End(RenderTask* task)
{
    GPUSwapChain::End(task);

    auto context = _device->GetMainContextDX12();

    // Indicate that the back buffer will be used to present a frame
    // Note: after that we should not use this backbuffer
    context->SetResourceState(&_backBuffers[_currentFrameIndex], D3D12_RESOURCE_STATE_PRESENT);

    // Send event
    context->OnSwapChainFlush();
}

void GPUSwapChainDX12::Present(bool vsync)
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    ID3D12Resource* backBuffer = _backBuffers[_currentFrameIndex].GetResource();
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
    planeParameters.Token = _framePipelineToken;
    planeParameters.ResourceCount = 1;
    planeParameters.ppResources = &backBuffer;
    VALIDATE_DIRECTX_CALL(_device->GetCommandQueueDX12()->PresentX(1, &planeParameters, nullptr));

    // Base
    GPUSwapChain::Present(vsync);

    // Switch to next back buffer
    _currentFrameIndex = (_currentFrameIndex + 1) % DX12_BACK_BUFFER_COUNT;
#else
    // Present frame
    UINT presentFlags = 0;
    if (!vsync && !_isFullscreen && _allowTearing)
    {
        presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }
    const HRESULT res = _swapChain->Present(vsync ? 1 : 0, presentFlags);
    LOG_DIRECTX_RESULT(res);

    // Base
    GPUSwapChain::Present(vsync);

    // Switch to next back buffer
    _currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();
#endif
}

GPUTextureView* GPUSwapChainDX12::GetBackBufferView()
{
    return &_backBuffers[_currentFrameIndex].Handle;
}

#endif
