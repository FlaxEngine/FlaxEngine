// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceDX12.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "../IncludeDirectXHeaders.h"
#include "ResourceOwnerDX12.h"
#include "GPUTextureDX12.h"

#if GRAPHICS_API_DIRECTX12

class GPUSwapChainDX12;

/// <summary>
/// Represents a DirectX 12 swap chain back buffer wrapper object.
/// </summary>
class BackBufferDX12 : public ResourceOwnerDX12
{
public:

    /// <summary>
    /// The render target surface handle.
    /// </summary>
    GPUTextureViewDX12 Handle;

public:

    /// <summary>
    /// Setup backbuffer wrapper
    /// </summary>
    /// <param name="window">Parent window</param>
    /// <param name="backbuffer">Back buffer DirectX 12 resource</param>
    void Setup(GPUSwapChainDX12* window, ID3D12Resource* backbuffer);

    /// <summary>
    /// Release references to the backbuffer
    /// </summary>
    void Release();

public:

    // [ResourceOwnerDX12]
    GPUResource* AsGPUResource() const override
    {
        return nullptr;
    }
};

/// <summary>
/// Graphics Device rendering output for DirectX 12 backend.
/// </summary>
class GPUSwapChainDX12 : public GPUResourceDX12<GPUSwapChain>
{
    friend class WindowsWindow;
    friend class GPUContextDX12;
    friend GPUDeviceDX12;

private:

    bool _allowTearing, _isFullscreen;
    HWND _windowHandle;
    IDXGISwapChain3* _swapChain;
    int32 _currentFrameIndex;
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    D3D12XBOX_FRAME_PIPELINE_TOKEN _framePipelineToken;
#endif
    Array<BackBufferDX12, FixedAllocation<4>> _backBuffers;

public:

    GPUSwapChainDX12(GPUDeviceDX12* device, Window* window);

public:

    /// <summary>
    /// Gets current backbuffer resource.
    /// </summary>
    /// <returns>The backbuffer resource.</returns>
    ID3D12Resource* GetBackBuffer() const
    {
        return _backBuffers[_currentFrameIndex].GetResource();
    }

    /// <summary>
    /// Gets render target handle for DirectX 12 backend.
    /// </summary>
    /// <returns>The render target handle.</returns>
    const GPUTextureViewDX12* GetBackBufferHandleDX12() const
    {
        return &_backBuffers[_currentFrameIndex].Handle;
    }

private:

    void getBackBuffer();
    void releaseBackBuffer();

public:

    // [GPUSwapChain]
    bool IsFullscreen() override;
    void SetFullscreen(bool isFullscreen) override;
    GPUTextureView* GetBackBufferView() override;
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    void Begin(RenderTask* task) override;
#endif
    void End(RenderTask* task) override;
    void Present(bool vsync) override;
    bool Resize(int32 width, int32 height) override;
    void CopyBackbuffer(GPUContext* context, GPUTexture* dst) override;

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
