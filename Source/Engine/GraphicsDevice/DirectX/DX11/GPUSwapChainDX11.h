// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceDX11.h"
#include "GPUTextureDX11.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// Graphics Device rendering output for DirectX 11 backend.
/// </summary>
class GPUSwapChainDX11 : public GPUResourceDX11<GPUSwapChain>
{
    friend class WindowsWindow;
    friend class GPUContextDX11;
    friend GPUDeviceDX11;

private:

#if PLATFORM_WINDOWS
    HWND _windowHandle;
    IDXGISwapChain* _swapChain;
    bool _allowTearing, _isFullscreen;
#else
	IUnknown* _windowHandle;
	IDXGISwapChain1* _swapChain;
#endif
    ID3D11Texture2D* _backBuffer;
    GPUTextureViewDX11 _backBufferHandle;

public:

    GPUSwapChainDX11(GPUDeviceDX11* device, Window* window);

private:

    void getBackBuffer();
    void releaseBackBuffer();

public:

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() override;

    // [GPUSwapChain]
    bool IsFullscreen() override;
    void SetFullscreen(bool isFullscreen) override;
    GPUTextureView* GetBackBufferView() override;
    void Present(bool vsync) override;
    bool Resize(int32 width, int32 height) override;
    void CopyBackbuffer(GPUContext* context, GPUTexture* dst) override;

protected:

    // [GPUResourceDX11]
    void OnReleaseGPU() final override;
};

#endif
