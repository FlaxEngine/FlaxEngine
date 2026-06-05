// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceWebGPU.h"
#include "GPUTextureWebGPU.h"
#include "Engine/Graphics/GPUSwapChain.h"

#if GRAPHICS_API_WEBGPU

/// <summary>
/// Graphics Device rendering output for Web GPU backend.
/// </summary>
class GPUSwapChainWebGPU : public GPUResourceWebGPU<GPUSwapChain>
{
    friend class WindowsWindow;
    friend class GPUContextWebGPU;
    friend GPUDeviceWebGPU;
private:
    GPUTextureViewWebGPU _surfaceView;

public:
    GPUSwapChainWebGPU(GPUDeviceWebGPU* device, Window* window);

public:
    WGPUSurface Surface = nullptr;

public:
    // [GPUSwapChain]
    bool IsFullscreen() override;
    void SetFullscreen(bool isFullscreen) override;
    GPUTextureView* GetBackBufferView() override;
    void Present(bool vsync) override;
    bool Resize(int32 width, int32 height) override;

protected:
    // [GPUResourceWebGPU]
    void OnReleaseGPU() final override;
};

#endif
