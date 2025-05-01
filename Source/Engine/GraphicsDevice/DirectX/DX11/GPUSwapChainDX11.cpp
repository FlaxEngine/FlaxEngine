// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUSwapChainDX11.h"
#include "Engine/Platform/Window.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "GPUContextDX11.h"

GPUSwapChainDX11::GPUSwapChainDX11(GPUDeviceDX11* device, Window* window)
    : GPUResourceDX11(device, StringView::Empty)
#if PLATFORM_WINDOWS
    , _windowHandle(static_cast<HWND>(window->GetNativePtr()))
#else
    , _windowHandle(static_cast<IUnknown*>(window->GetNativePtr()))
#endif
    , _swapChain(nullptr)
#if PLATFORM_WINDOWS
    , _allowTearing(false)
    , _isFullscreen(false)
#endif
    , _backBuffer(nullptr)
{
    ASSERT(_windowHandle);
    _window = window;
}

void GPUSwapChainDX11::getBackBuffer()
{
    VALIDATE_DIRECTX_CALL(_swapChain->GetBuffer(0, __uuidof(_backBuffer), reinterpret_cast<void**>(&_backBuffer)));

    ID3D11RenderTargetView* rtv;
    ID3D11ShaderResourceView* srv;
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateRenderTargetView(_backBuffer, nullptr, &rtv));
#if GPU_USE_WINDOW_SRV
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateShaderResourceView(_backBuffer, nullptr, &srv));
#else
	srv = nullptr;
#endif

    _backBufferHandle.Init(this, rtv, srv, nullptr, nullptr, _format, MSAALevel::None);
}

void GPUSwapChainDX11::releaseBackBuffer()
{
    // Release data
    _backBufferHandle.Release();
    DX_SAFE_RELEASE_CHECK(_backBuffer, 0);
}

void GPUSwapChainDX11::OnReleaseGPU()
{
#if PLATFORM_WINDOWS
    // Disable fullscreen mode
    if (_swapChain)
    {
        VALIDATE_DIRECTX_CALL(_swapChain->SetFullscreenState(false, nullptr));
    }
#endif

    // Release data
    releaseBackBuffer();
    DX_SAFE_RELEASE_CHECK(_swapChain, 0);
    _width = _height = 0;
}

ID3D11Resource* GPUSwapChainDX11::GetResource()
{
    return _backBuffer;
}

bool GPUSwapChainDX11::IsFullscreen()
{
    // Check if has swap chain created
    if (_swapChain == nullptr)
        return false;

    // Get state
    BOOL state;
    VALIDATE_DIRECTX_CALL(_swapChain->GetFullscreenState(&state, nullptr));
    return state == TRUE;
}

void GPUSwapChainDX11::SetFullscreen(bool isFullscreen)
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

GPUTextureView* GPUSwapChainDX11::GetBackBufferView()
{
    return &_backBufferHandle;
}

void GPUSwapChainDX11::Present(bool vsync)
{
    // Present frame
    ASSERT(_swapChain);
    UINT presentFlags = 0;
#if PLATFORM_WINDOWS
    if (!vsync && !_isFullscreen && _allowTearing)
    {
        presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }
#endif
    const HRESULT result = _swapChain->Present(vsync ? 1 : 0, presentFlags);
    LOG_DIRECTX_RESULT(result);

    // Base
    GPUSwapChain::Present(vsync);
}

bool GPUSwapChainDX11::Resize(int32 width, int32 height)
{
    // Check if size won't change
    if (width == _width && height == _height)
    {
        return false;
    }

    _device->WaitForGPU();
    GPUDeviceLock lock(_device);
#if PLATFORM_WINDOWS
    _allowTearing = _device->_allowTearing;
#endif
    _format = GPU_BACK_BUFFER_PIXEL_FORMAT;

#if PLATFORM_WINDOWS
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
#else
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
#endif
    if (_swapChain == nullptr)
    {
        ReleaseGPU();

        // Create swap chain description
#if PLATFORM_WINDOWS
        if (_device->Outputs.HasItems())
        {
            const uint32 outputIdx = 0;
            auto& output = _device->Outputs[outputIdx];
            swapChainDesc.BufferDesc = output.DesktopViewMode;
        }
        else
        {
            swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
            swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
            swapChainDesc.BufferDesc.Format = RenderToolsDX::ToDxgiFormat(_format);
            swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        }
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.OutputWindow = _windowHandle;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (_allowTearing)
        {
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
#else
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = RenderToolsDX::ToDxgiFormat(_format);
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
#endif
#if GPU_USE_WINDOW_SRV
        swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
#endif

        // Create swap chain
#if PLATFORM_WINDOWS
        auto dxgi = _device->GetDXGIFactory();
        VALIDATE_DIRECTX_CALL(dxgi->CreateSwapChain(_device->GetDevice(), &swapChainDesc, &_swapChain));
        ASSERT(_swapChain);

        // Disable DXGI changes to the window
        VALIDATE_DIRECTX_CALL(dxgi->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER));
#else
        auto dxgiFactory = (IDXGIFactory2*)_device->GetDXGIFactory();
        VALIDATE_DIRECTX_CALL(dxgiFactory->CreateSwapChainForCoreWindow(_device->GetDevice(), static_cast<IUnknown*>(_windowHandle), &swapChainDesc, nullptr, &_swapChain));
        ASSERT(_swapChain);

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        ComPtr<IDXGIDevice2> dxgiDevice;
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
        VALIDATE_DIRECTX_CALL(dxgiDevice->SetMaximumFrameLatency(1));
#endif
    }
    else
    {
        releaseBackBuffer();

#if PLATFORM_WINDOWS
        _swapChain->GetDesc(&swapChainDesc);
        VALIDATE_DIRECTX_CALL(_swapChain->ResizeBuffers(swapChainDesc.BufferCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
#else
        _swapChain->GetDesc1(&swapChainDesc);
        VALIDATE_DIRECTX_CALL(_swapChain->ResizeBuffers(swapChainDesc.BufferCount, width, height, swapChainDesc.Format, swapChainDesc.Flags));
#endif
    }

    _width = width;
    _height = height;
    _memoryUsage = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * swapChainDesc.BufferCount;

    getBackBuffer();

    return false;
}

void GPUSwapChainDX11::CopyBackbuffer(GPUContext* context, GPUTexture* dst)
{
    const auto contextDX11 = (GPUContextDX11*)context;
    contextDX11->GetContext()->CopyResource((ID3D11Resource*)dst->GetNativePtr(), _backBuffer);
}

#endif
