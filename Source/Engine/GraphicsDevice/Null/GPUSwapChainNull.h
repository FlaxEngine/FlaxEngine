// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUSwapChain.h"

/// <summary>
/// Graphics Device rendering output for Null backend.
/// </summary>
class GPUSwapChainNull : public GPUSwapChain
{
public:

    GPUSwapChainNull(Window* window)
        : GPUSwapChain()
    {
        _window = window;
    }

public:

    // [GPUSwapChain]
    bool IsFullscreen() override
    {
        return false;
    }

    void SetFullscreen(bool isFullscreen) override
    {
    }

    GPUTextureView* GetBackBufferView() override
    {
        return nullptr;
    }

    void Present(bool vsync) override
    {
    }

    bool Resize(int32 width, int32 height) override
    {
        return false;
    }

    void CopyBackbuffer(GPUContext* context, GPUTexture* dst) override
    {
    }
};

#endif
