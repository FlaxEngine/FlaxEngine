// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUSwapChainDX11.h"
#include "GPUContextDX11.h"
#include "Engine/Graphics/Graphics.h"
#include <atomic>
#include "Engine/Platform/Window.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

#if PLATFORM_WINDOWS
// The render device/context stays on the render thread. The worker exclusively
// owns a second device/context on the same adapter. Keyed mutexes synchronize
// cross-device GPU copies; Published transfers CPU ownership of each slot.
struct GPUSwapChainDX11::AsyncPresenter
{
    struct Frame
    {
        ID3D11Texture2D* RenderTexture = nullptr;
        ID3D11Texture2D* PresentTexture = nullptr;
        IDXGIKeyedMutex* RenderMutex = nullptr;
        IDXGIKeyedMutex* PresentMutex = nullptr;
        std::atomic<bool> Published{false};
        uint64 Serial = 0;
    };
    GPUSwapChainDX11* Owner;
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* Context = nullptr;
    ID3D11Texture2D* RenderTarget = nullptr;
    GPUTextureViewDX11 RenderView;
    Frame Frames[2];
    HANDLE Thread = nullptr, StopEvent = nullptr, WakeEvent = nullptr;
    std::atomic<uint64> Presented{0};
    std::atomic<HRESULT> Failure{S_OK};
    uint64 Serial = 0;
    bool FailureReported = false;

    explicit AsyncPresenter(GPUSwapChainDX11* owner) : Owner(owner) {}
    ~AsyncPresenter()
    {
        ReleaseFrames();
        SAFE_RELEASE(Context);
        SAFE_RELEASE(Device);
    }
    bool InitDevice()
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        ComPtr<IDXGIAdapter> adapter;
        if (FAILED(Owner->_device->GetDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice))) || FAILED(dxgiDevice->GetAdapter(&adapter)))
            return false;
        D3D_FEATURE_LEVEL level;
        if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &Device, &level, &Context)))
            return false;
        ComPtr<IDXGIDevice1> presentationDevice;
        return SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&presentationDevice))) &&
            SUCCEEDED(presentationDevice->SetMaximumFrameLatency(1));
    }
    void Stop()
    {
        if (Thread)
        {
            SetEvent(StopEvent);
            if (GetCurrentThreadId() == GetWindowThreadProcessId(Owner->_windowHandle, nullptr))
            {
                // DXGI may send synchronous window messages even with DO_NOT_WAIT.
                // Service sent messages while joining, without dispatching posted input.
                while (MsgWaitForMultipleObjects(1, &Thread, FALSE, INFINITE, QS_SENDMESSAGE) == WAIT_OBJECT_0 + 1)
                {
                    MSG message;
                    PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE);
                }
            }
            else
                WaitForSingleObject(Thread, INFINITE);
            CloseHandle(Thread);
            Thread = nullptr;
        }
        if (StopEvent) CloseHandle(StopEvent);
        if (WakeEvent) CloseHandle(WakeEvent);
        StopEvent = WakeEvent = nullptr;
    }
    void ReleaseFrames()
    {
        Stop();
        if (Context)
        {
            Context->ClearState();
            // The engine's render-device fence does not cover this second device.
            // Drain it on lifecycle changes before releasing swapchain resources.
            ID3D11Query* done = nullptr;
            const D3D11_QUERY_DESC desc = {D3D11_QUERY_EVENT, 0};
            if (SUCCEEDED(Device->CreateQuery(&desc, &done)))
            {
                Context->End(done);
                Context->Flush();
                while (Context->GetData(done, nullptr, 0, 0) == S_FALSE)
                    Platform::Sleep(1);
                done->Release();
            }
            else
                Context->Flush();
        }
        RenderView.Release();
        SAFE_RELEASE(RenderTarget);
        for (auto& frame : Frames)
        {
            SAFE_RELEASE(frame.PresentMutex);
            SAFE_RELEASE(frame.RenderMutex);
            SAFE_RELEASE(frame.PresentTexture);
            SAFE_RELEASE(frame.RenderTexture);
            frame.Published.store(false);
        }
    }
    bool InitFrames()
    {
        Failure.store(S_OK);
        FailureReported = false;
        Presented.store(0);
        D3D11_TEXTURE2D_DESC desc;
        Owner->_backBuffer->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
#if GPU_ENABLE_WINDOW_SRV
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
#endif
        auto renderDevice = Owner->_device->GetDevice();
        if (FAILED(renderDevice->CreateTexture2D(&desc, nullptr, &RenderTarget)))
            return false;
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(renderDevice->CreateRenderTargetView(RenderTarget, nullptr, &rtv)))
            return false;
#if GPU_ENABLE_WINDOW_SRV
        if (FAILED(renderDevice->CreateShaderResourceView(RenderTarget, nullptr, &srv)))
        {
            SAFE_RELEASE(rtv);
            return false;
        }
#endif
        RenderView.Init(Owner, rtv, srv, nullptr, nullptr, Owner->_format, MSAALevel::None);
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        for (auto& frame : Frames)
        {
            ComPtr<IDXGIResource> resource;
            HANDLE shared = nullptr;
            if (FAILED(renderDevice->CreateTexture2D(&desc, nullptr, &frame.RenderTexture)) ||
                FAILED(frame.RenderTexture->QueryInterface(IID_PPV_ARGS(&frame.RenderMutex))) ||
                FAILED(frame.RenderTexture->QueryInterface(IID_PPV_ARGS(&resource))) ||
                FAILED(resource->GetSharedHandle(&shared)) ||
                FAILED(Device->OpenSharedResource(shared, IID_PPV_ARGS(&frame.PresentTexture))) ||
                FAILED(frame.PresentTexture->QueryInterface(IID_PPV_ARGS(&frame.PresentMutex))))
                return false;
            // Legacy shared handles returned by GetSharedHandle must not be closed.
        }
        StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        WakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!StopEvent || !WakeEvent)
            return false;
        Thread = CreateThread(nullptr, 0, Run, this, 0, nullptr);
        if (Thread)
            LOG(Info, "DX11 async presentation active: separate presentation device, two shared frame slots, synchronized scanout.");
        return Thread != nullptr;
    }
    void Capture()
    {
        const HRESULT failure = Failure.load();
        if (FAILED(failure))
        {
            if (!FailureReported)
            {
                LOG_DIRECTX_RESULT(failure);
                FailureReported = true;
            }
            return;
        }
        // Base bookkeeping (including window visibility) stays on the render thread.
        const uint64 count = Presented.exchange(0);
        if (count)
        {
            Owner->GPUSwapChain::Present(true);
            Owner->_presentCount += count - 1;
        }
        auto graphicsContext = Owner->_device->GetMainContextDX11();
        graphicsContext->ResetRenderTarget();
        graphicsContext->FlushState();
        for (auto& frame : Frames)
        {
            if (frame.Published.load(std::memory_order_acquire))
                continue;
            // Zero timeout: never wait for a slot still used by the GPU.
            const HRESULT acquired = frame.RenderMutex->AcquireSync(0, 0);
            if (acquired != S_OK)
            {
                if (acquired != WAIT_TIMEOUT)
                    Failure.store(FAILED(acquired) ? acquired : E_FAIL);
                continue;
            }
            graphicsContext->GetContext()->CopyResource(frame.RenderTexture, RenderTarget);
            const HRESULT released = frame.RenderMutex->ReleaseSync(1);
            if (FAILED(released))
            {
                Failure.store(released);
                return;
            }
            graphicsContext->GetContext()->Flush();
            frame.Serial = ++Serial;
            frame.Published.store(true, std::memory_order_release);
            SetEvent(WakeEvent);
            return;
        }
        // Both slots occupied: discard this capture and keep rendering.
    }
    static DWORD WINAPI Run(void* arg)
    {
        auto self = static_cast<AsyncPresenter*>(arg);
        HANDLE events[] = {self->StopEvent, self->WakeEvent};
        bool pending = false;
        uint64 lastPresented = 0;
        uint64 pendingSerial = 0;
        while (WaitForMultipleObjects(2, events, FALSE, 1) != WAIT_OBJECT_0)
        {
            Frame* newest = nullptr;
            for (auto& frame : self->Frames)
            {
                if (!frame.Published.load(std::memory_order_acquire))
                    continue;
                const HRESULT acquired = frame.PresentMutex->AcquireSync(1, 0);
                if (acquired != S_OK)
                {
                    if (acquired != WAIT_TIMEOUT) self->Failure.store(FAILED(acquired) ? acquired : E_FAIL);
                    continue;
                }
                if (frame.Serial <= lastPresented || (pending && frame.Serial <= pendingSerial) || (newest && frame.Serial < newest->Serial))
                {
                    const HRESULT released = frame.PresentMutex->ReleaseSync(0);
                    if (FAILED(released)) self->Failure.store(released);
                    else frame.Published.store(false, std::memory_order_release);
                    continue;
                }
                if (newest)
                {
                    const HRESULT released = newest->PresentMutex->ReleaseSync(0);
                    if (FAILED(released)) self->Failure.store(released);
                    else newest->Published.store(false, std::memory_order_release);
                }
                newest = &frame;
            }
            if (FAILED(self->Failure.load()))
                break;
            if (newest)
            {
                self->Context->CopyResource(self->Owner->_backBuffer, newest->PresentTexture);
                pendingSerial = newest->Serial;
                const HRESULT released = newest->PresentMutex->ReleaseSync(0);
                if (FAILED(released))
                {
                    self->Failure.store(released);
                    break;
                }
                self->Context->Flush();
                newest->Published.store(false, std::memory_order_release);
                pending = true;
            }
            if (pending)
            {
                // Synchronized scanout, but do not block teardown behind a full queue.
                const HRESULT result = self->Owner->_swapChain->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
                if (result == S_OK)
                {
                    lastPresented = pendingSerial;
                    self->Presented.fetch_add(1);
                    pending = false;
                }
                else if (result == DXGI_STATUS_OCCLUDED)
                {
                    WaitForSingleObject(self->StopEvent, 16);
                }
                else if (result != DXGI_ERROR_WAS_STILL_DRAWING)
                {
                    self->Failure.store(result);
                    break;
                }
            }
        }
        return 0;
    }
};
#endif

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

#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        return;
#endif
    ID3D11RenderTargetView* rtv;
    ID3D11ShaderResourceView* srv;
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateRenderTargetView(_backBuffer, nullptr, &rtv));
#if GPU_ENABLE_WINDOW_SRV
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateShaderResourceView(_backBuffer, nullptr, &srv));
#else
	srv = nullptr;
#endif

    _backBufferHandle.Init(this, rtv, srv, nullptr, nullptr, _format, MSAALevel::None);
}

void GPUSwapChainDX11::releaseBackBuffer()
{
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        _asyncPresenter->ReleaseFrames();
#endif
    // Release data
    _backBufferHandle.Release();
    DX_SAFE_RELEASE_CHECK(_backBuffer, 0);
}

void GPUSwapChainDX11::OnReleaseGPU()
{
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        _asyncPresenter->Stop();
    // Disable fullscreen mode
    if (_swapChain && _isFullscreen)
    {
        VALIDATE_DIRECTX_CALL(_swapChain->SetFullscreenState(false, nullptr));
    }
#endif

    // Release data
    PROFILE_MEM_DEC(Graphics, _memoryUsage);
    releaseBackBuffer();
    DX_SAFE_RELEASE_CHECK(_swapChain, 0);
#if PLATFORM_WINDOWS
    delete _asyncPresenter;
    _asyncPresenter = nullptr;
#endif
    _width = _height = 0;
    _memoryUsage = 0;
}

ID3D11Resource* GPUSwapChainDX11::GetResource()
{
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        return _asyncPresenter->RenderTarget;
#endif
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
    if (isFullscreen && _asyncPresenter)
    {
        LOG(Warning, "Async presentation is windowed/borderless only.");
        return;
    }
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
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        return &_asyncPresenter->RenderView;
#endif
    return &_backBufferHandle;
}

void GPUSwapChainDX11::Present(bool vsync)
{
    PROFILE_CPU();
    ZoneColor(TracyWaitZoneColor);

#if PLATFORM_WINDOWS
    if (_asyncPresenter)
    {
        _asyncPresenter->Capture();
        return;
    }
#endif
    // Present frame
    ASSERT(_swapChain);
    UINT presentFlags = 0;
#if PLATFORM_WINDOWS
    if (!vsync && !_isFullscreen && _allowTearing)
    {
        presentFlags |= DXGI_PRESENT_DO_NOT_WAIT;
    }
#endif
    const HRESULT result = _swapChain->Present(vsync ? 1 : 0, presentFlags);
    // Nonblocking test: a busy presentation queue is not a device failure.
    if (result == DXGI_ERROR_WAS_STILL_DRAWING)
        return;
    LOG_DIRECTX_RESULT(result);

    // Base
    GPUSwapChain::Present(vsync);
}

bool GPUSwapChainDX11::Resize(int32 width, int32 height)
{
    if (width == _width && height == _height)
        return false;

#if PLATFORM_WINDOWS
    if (_asyncPresenter)
        _asyncPresenter->Stop();
#endif
    _device->WaitForGPU();
    GPUDeviceLock lock(_device);
#if PLATFORM_WINDOWS
    _allowTearing = _device->_allowTearing;
#endif
    _format = GPU_BACK_BUFFER_PIXEL_FORMAT;
    if (_memoryUsage != 0)
    {
        PROFILE_MEM_DEC(Graphics, _memoryUsage);
    }

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
#if GPU_ENABLE_WINDOW_SRV
        swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
#endif

        // Create swap chain
#if PLATFORM_WINDOWS
        auto dxgi = _device->GetDXGIFactory();
        ID3D11Device* presentationDevice = _device->GetDevice();
        if (Graphics::UseAsyncPresentation)
        {
            _asyncPresenter = new AsyncPresenter(this);
            if (_asyncPresenter->InitDevice())
            {
                presentationDevice = _asyncPresenter->Device;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapChainDesc.Flags = 0;
            }
            else
            {
                delete _asyncPresenter;
                _asyncPresenter = nullptr;
                LOG(Warning, "Async presentation device unavailable; using normal DX11 presentation.");
            }
        }
        VALIDATE_DIRECTX_CALL(dxgi->CreateSwapChain(presentationDevice, &swapChainDesc, &_swapChain));
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
    PROFILE_MEM_INC(Graphics, _memoryUsage);

    getBackBuffer();
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
    {
        if (!_asyncPresenter->InitFrames())
        {
            LOG(Error, "Async presentation frame setup failed.");
            ReleaseGPU();
            return true;
        }
        const uint64 extra = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * 3;
        _memoryUsage += extra;
        PROFILE_MEM_INC(Graphics, extra);
    }
#endif

    return false;
}

void GPUSwapChainDX11::CopyBackbuffer(GPUContext* context, GPUTexture* dst)
{
    const auto contextDX11 = (GPUContextDX11*)context;
    contextDX11->GetContext()->CopyResource((ID3D11Resource*)dst->GetNativePtr(), GetResource());
}

#endif
