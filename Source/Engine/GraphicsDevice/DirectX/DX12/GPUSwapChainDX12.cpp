// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUSwapChainDX12.h"
#include "GPUContextDX12.h"
#include "Engine/Graphics/Graphics.h"
#include <atomic>
#include "../IncludeDirectXHeaders.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

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

#if GPU_ENABLE_WINDOW_SRV
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

#if PLATFORM_WINDOWS
struct GPUSwapChainDX12::AsyncPresenter
{
    struct Frame
    {
        ID3D12Resource* Texture = nullptr;
        ID3D12CommandAllocator* CaptureAllocator = nullptr;
        ID3D12GraphicsCommandList* CaptureList = nullptr;
        ID3D12CommandAllocator* PresentAllocator = nullptr;
        ID3D12GraphicsCommandList* PresentList = nullptr;
        // 0: free; 1: published render copy; 2: presentation GPU copy in flight.
        std::atomic<int> State{0};
        uint64 Serial = 0, CopyComplete = 0;
    };
    GPUSwapChainDX12* Owner;
    ID3D12CommandQueue* Queue = nullptr;
    ID3D12Fence* RenderFence = nullptr;
    ID3D12Fence* CopyFence = nullptr;
    Frame Frames[2];
    HANDLE Thread = nullptr, StopEvent = nullptr, WakeEvent = nullptr;
    std::atomic<uint64> Presented{0};
    std::atomic<HRESULT> Failure{S_OK};
    uint64 RenderSerial = 0, CopySerial = 0;
    bool FailureReported = false;

    explicit AsyncPresenter(GPUSwapChainDX12* owner) : Owner(owner) {}
    ~AsyncPresenter()
    {
        ReleaseFrames();
        SAFE_RELEASE(RenderFence);
        SAFE_RELEASE(CopyFence);
        SAFE_RELEASE(Queue);
    }
    bool InitQueue()
    {
        auto device = Owner->_device->GetDevice();
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        return SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&Queue))) &&
            SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&RenderFence))) &&
            SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CopyFence)));
    }
    void Stop()
    {
        if (Thread)
        {
            SetEvent(StopEvent);
            if (GetCurrentThreadId() == GetWindowThreadProcessId(Owner->_windowHandle, nullptr))
            {
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
        // The engine queue fence does not cover the presentation queue.
        if (Queue && CopyFence && SUCCEEDED(Queue->Signal(CopyFence, ++CopySerial)))
        {
            uint64 completed;
            while ((completed = CopyFence->GetCompletedValue()) != MAX_uint64 && completed < CopySerial)
                Platform::Sleep(1);
        }
        Owner->_asyncRenderTarget.Release();
        for (auto& frame : Frames)
        {
            SAFE_RELEASE(frame.CaptureList);
            SAFE_RELEASE(frame.PresentList);
            SAFE_RELEASE(frame.CaptureAllocator);
            SAFE_RELEASE(frame.PresentAllocator);
            SAFE_RELEASE(frame.Texture);
            frame.State.store(0);
        }
    }
    static D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        return barrier;
    }
    bool InitFrames()
    {
        auto device = Owner->_device->GetDevice();
        auto desc = Owner->_backBuffers[0].GetResource()->GetDesc();
        D3D12_HEAP_PROPERTIES heap = {};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap.CreationNodeMask = heap.VisibleNodeMask = 1;
        ID3D12Resource* target = nullptr;
        if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&target))))
            return false;
        Owner->_asyncRenderTarget.Setup(Owner, target);
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        for (auto& frame : Frames)
        {
            if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                    D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&frame.Texture))) ||
                FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.CaptureAllocator))) ||
                FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.PresentAllocator))) ||
                FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.CaptureAllocator, nullptr, IID_PPV_ARGS(&frame.CaptureList))) ||
                FAILED(frame.CaptureList->Close()) ||
                FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.PresentAllocator, nullptr, IID_PPV_ARGS(&frame.PresentList))) ||
                FAILED(frame.PresentList->Close()))
                return false;
        }
        Failure.store(S_OK);
        FailureReported = false;
        Presented.store(0);
        StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        WakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!StopEvent || !WakeEvent)
            return false;
        Thread = CreateThread(nullptr, 0, Run, this, 0, nullptr);
        if (Thread)
            LOG(Info, "DX12 async presentation active: separate queue, two fenced frame slots, synchronized scanout.");
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
        const uint64 count = Presented.exchange(0);
        if (count)
        {
            Owner->GPUSwapChain::Present(true);
            Owner->_presentCount += count - 1;
        }
        // Present is called after the engine's FrameEnd submitted its draw lists.
        for (auto& frame : Frames)
        {
            if (frame.State.load(std::memory_order_acquire) != 0)
                continue;
            HRESULT result = frame.CaptureAllocator->Reset();
            if (SUCCEEDED(result)) result = frame.CaptureList->Reset(frame.CaptureAllocator, nullptr);
            if (FAILED(result)) { Failure.store(result); return; }
            auto target = Owner->_asyncRenderTarget.GetResource();
            D3D12_RESOURCE_BARRIER begin[] = {
                Transition(target, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
                Transition(frame.Texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)};
            frame.CaptureList->ResourceBarrier(2, begin);
            frame.CaptureList->CopyResource(frame.Texture, target);
            D3D12_RESOURCE_BARRIER end[] = {
                Transition(target, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON),
                Transition(frame.Texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON)};
            frame.CaptureList->ResourceBarrier(2, end);
            result = frame.CaptureList->Close();
            if (FAILED(result)) { Failure.store(result); return; }
            auto queue = Owner->_device->GetCommandQueueDX12();
            ID3D12CommandList* list = frame.CaptureList;
            queue->ExecuteCommandLists(1, &list);
            frame.Serial = ++RenderSerial;
            result = queue->Signal(RenderFence, frame.Serial);
            if (FAILED(result)) { Failure.store(result); return; }
            frame.State.store(1, std::memory_order_release);
            SetEvent(WakeEvent);
            return;
        }
        // No safe slot: drop this capture without stalling rendering.
    }
    static DWORD WINAPI Run(void* arg)
    {
        auto self = static_cast<AsyncPresenter*>(arg);
        HANDLE events[] = {self->StopEvent, self->WakeEvent};
        bool pending = false;
        uint64 pendingSerial = 0, lastPresented = 0;
        while (WaitForMultipleObjects(2, events, FALSE, 1) != WAIT_OBJECT_0)
        {
            const uint64 rendered = self->RenderFence->GetCompletedValue();
            const uint64 copied = self->CopyFence->GetCompletedValue();
            if (rendered == MAX_uint64 || copied == MAX_uint64)
            {
                self->Failure.store(DXGI_ERROR_DEVICE_REMOVED);
                break;
            }
            Frame* newest = nullptr;
            for (auto& frame : self->Frames)
            {
                int state = frame.State.load(std::memory_order_acquire);
                if (state == 2 && copied >= frame.CopyComplete)
                {
                    frame.State.store(0, std::memory_order_release);
                    continue;
                }
                if (state != 1 || rendered < frame.Serial)
                    continue;
                if (frame.Serial <= lastPresented || (pending && frame.Serial <= pendingSerial))
                {
                    frame.State.store(0, std::memory_order_release);
                    continue;
                }
                if (!newest || frame.Serial > newest->Serial)
                    newest = &frame;
            }
            if (newest)
            {
                for (auto& frame : self->Frames)
                {
                    if (&frame != newest && frame.State.load(std::memory_order_acquire) == 1 &&
                        rendered >= frame.Serial && frame.Serial < newest->Serial)
                        frame.State.store(0, std::memory_order_release);
                }
                HRESULT result = newest->PresentAllocator->Reset();
                if (SUCCEEDED(result)) result = newest->PresentList->Reset(newest->PresentAllocator, nullptr);
                if (FAILED(result)) { self->Failure.store(result); break; }
                const UINT index = self->Owner->_swapChain->GetCurrentBackBufferIndex();
                auto back = self->Owner->_backBuffers[index].GetResource();
                D3D12_RESOURCE_BARRIER begin[] = {
                    Transition(newest->Texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
                    Transition(back, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)};
                newest->PresentList->ResourceBarrier(2, begin);
                newest->PresentList->CopyResource(back, newest->Texture);
                D3D12_RESOURCE_BARRIER end[] = {
                    Transition(newest->Texture, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON),
                    Transition(back, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT)};
                newest->PresentList->ResourceBarrier(2, end);
                result = newest->PresentList->Close();
                if (FAILED(result)) { self->Failure.store(result); break; }
                ID3D12CommandList* list = newest->PresentList;
                self->Queue->ExecuteCommandLists(1, &list);
                newest->CopyComplete = ++self->CopySerial;
                result = self->Queue->Signal(self->CopyFence, newest->CopyComplete);
                if (FAILED(result)) { self->Failure.store(result); break; }
                pendingSerial = newest->Serial;
                newest->State.store(2, std::memory_order_release);
                pending = true;
            }
            if (pending)
            {
                const HRESULT result = self->Owner->_swapChain->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
                if (result == S_OK)
                {
                    lastPresented = pendingSerial;
                    self->Presented.fetch_add(1);
                    pending = false;
                }
                else if (result == DXGI_STATUS_OCCLUDED)
                    WaitForSingleObject(self->StopEvent, 16);
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
#if PLATFORM_WINDOWS
    if (_asyncPresenter) _asyncPresenter->Stop();
#endif
    _device->WaitForGPU();

#if PLATFORM_WINDOWS
    // Disable fullscreen mode
    if (_swapChain && _isFullscreen)
    {
        VALIDATE_DIRECTX_CALL(_swapChain->SetFullscreenState(false, nullptr));
    }
#endif

    // Release data
    PROFILE_MEM_DEC(Graphics, _memoryUsage);
    releaseBackBuffer();
    _backBuffers.Resize(0);
#if PLATFORM_WINDOWS
    delete _asyncPresenter;
    _asyncPresenter = nullptr;
#endif
    if (_swapChain)
    {
        _device->AddResourceToLateRelease(_swapChain);
        _swapChain = nullptr;
    }
    _width = _height = 0;
    _memoryUsage = 0;
}

void GPUSwapChainDX12::releaseBackBuffer()
{
#if PLATFORM_WINDOWS
    if (_asyncPresenter) _asyncPresenter->ReleaseFrames();
#endif
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

bool GPUSwapChainDX12::Resize(int32 width, int32 height)
{
    // Check if window state won't change
    if (width == _width && height == _height)
    {
        return false;
    }

#if PLATFORM_WINDOWS
    if (_asyncPresenter) _asyncPresenter->Stop();
#endif
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
#if GPU_ENABLE_WINDOW_SRV
        swapChainDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
#endif
        ID3D12CommandQueue* presentationQueue = _device->GetCommandQueueDX12();
#if PLATFORM_WINDOWS
        if (Graphics::UseAsyncPresentation)
        {
            _asyncPresenter = new AsyncPresenter(this);
            if (_asyncPresenter->InitQueue())
            {
                presentationQueue = _asyncPresenter->Queue;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            }
            else
            {
                delete _asyncPresenter;
                _asyncPresenter = nullptr;
                LOG(Warning, "Async presentation queue unavailable; using normal DX12 presentation.");
            }
        }
#endif
        if (_allowTearing
#if PLATFORM_WINDOWS
            && !_asyncPresenter
#endif
            )
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
        VALIDATE_DIRECTX_CALL(dxgiFactory->CreateSwapChainForHwnd(presentationQueue, _windowHandle, &swapChainDesc, &fullscreenDesc, nullptr, &swapChain));
        _swapChain = static_cast<IDXGISwapChain3*>(swapChain);
        ASSERT(_swapChain);
#if PLATFORM_WINDOWS
        if (_asyncPresenter)
            VALIDATE_DIRECTX_CALL(_swapChain->SetMaximumFrameLatency(1));
#endif
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
    PROFILE_MEM_INC(Graphics, _memoryUsage);

    getBackBuffer();
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
    {
        if (!_asyncPresenter->InitFrames())
        {
            LOG(Error, "Async DX12 presentation frame setup failed.");
            ReleaseGPU();
            return true;
        }
        const uint64 extra = RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * 3;
        _memoryUsage += extra;
        PROFILE_MEM_INC(Graphics, extra);
    }
#endif
#endif

    return false;
}

void GPUSwapChainDX12::CopyBackbuffer(GPUContext* context, GPUTexture* dst)
{
    const auto contextDX12 = (GPUContextDX12*)context;

    auto dstDX12 = (GPUTextureDX12*)dst;
    auto backbuffer = &_backBuffers[_currentFrameIndex];
#if PLATFORM_WINDOWS
    if (_asyncPresenter) backbuffer = &_asyncRenderTarget;
#endif

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
#if PLATFORM_WINDOWS
    if (_asyncPresenter)
    {
        context->OnSwapChainFlush(&_asyncRenderTarget);
        return;
    }
#endif
    context->OnSwapChainFlush(&_backBuffers[_currentFrameIndex]);
}

void GPUSwapChainDX12::Present(bool vsync)
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
#if GPU_ENABLE_ASSERTION
    //ASSERT(_currentFrameIndex == _swapChain->GetCurrentBackBufferIndex());
    ASSERT(_backBuffers[_currentFrameIndex].State.CheckResourceState(D3D12_RESOURCE_STATE_PRESENT));
#endif
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
#if PLATFORM_WINDOWS
    if (_asyncPresenter) return &_asyncRenderTarget.Handle;
#endif
    return &_backBuffers[_currentFrameIndex].Handle;
}

#endif
