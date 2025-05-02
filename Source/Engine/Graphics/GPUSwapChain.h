// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUResource.h"
#include "PixelFormat.h"
#include "Engine/Platform/Window.h"

class GPUContext;
class GPUTexture;
class GPUTextureView;
class Task;

/// <summary>
/// GPU swap chain object that provides rendering to native window backbuffer.
/// </summary>
class FLAXENGINE_API GPUSwapChain : public GPUResource
{
protected:
    int32 _width = 0;
    int32 _height = 0;
    uint64 _presentCount = 0;
    PixelFormat _format = PixelFormat::Unknown;
    Window* _window = nullptr;
    Task* _downloadTask = nullptr;

    GPUSwapChain();

public:
    /// <summary>
    /// Gets the linked window.
    /// </summary>
    Window* GetWindow() const
    {
        return _window;
    }

    /// <summary>
    /// The output backbuffer width (in pixels).
    /// </summary>
    FORCE_INLINE int32 GetWidth() const
    {
        return _width;
    }

    /// <summary>
    /// The output backbuffer height (in pixels).
    /// </summary>
    FORCE_INLINE int32 GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// The output backbuffer surface format.
    /// </summary>
    FORCE_INLINE PixelFormat GetFormat() const
    {
        return _format;
    }

    /// <summary>
    /// The output backbuffer width and height (in pixels).
    /// </summary>
    FORCE_INLINE Float2 GetSize() const
    {
        return Float2(static_cast<float>(_width), static_cast<float>(_height));
    }

    /// <summary>
    /// The output backbuffer aspect ratio.
    /// </summary>
    FORCE_INLINE float GetAspectRatio() const
    {
        return static_cast<float>(_width) / _height;
    }

    /// <summary>
    /// Gets amount of backbuffer swaps.
    /// </summary>
    FORCE_INLINE uint64 GetPresentCount() const
    {
        return _presentCount;
    }

    /// <summary>
    /// True if running in fullscreen mode.
    /// </summary>
    /// <returns>True if is in fullscreen mode, otherwise false</returns>
    virtual bool IsFullscreen() = 0;

    /// <summary>
    /// Set the fullscreen state.
    /// </summary>
    /// <param name="isFullscreen">Fullscreen mode to apply</param>
    virtual void SetFullscreen(bool isFullscreen) = 0;

    /// <summary>
    /// Gets the view for the output back buffer texture (for the current frame rendering).
    /// </summary>
    /// <returns>The output texture view to use.</returns>
    virtual GPUTextureView* GetBackBufferView() = 0;

    /// <summary>
    /// Copies the backbuffer contents to the destination texture.
    /// </summary>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="dst">The destination texture. It must match the output dimensions and format. No staging texture support.</param>
    virtual void CopyBackbuffer(GPUContext* context, GPUTexture* dst) = 0;

    /// <summary>
    /// Checks if task is ready to render.
    /// </summary>
    /// <returns>True if is ready, otherwise false</returns>
    virtual bool IsReady() const
    {
        // Skip rendering for the hidden windows
        return GetWidth() > 0 && (_window->IsVisible() || _window->_showAfterFirstPaint);
    }

public:
    /// <summary>
    /// Creates GPU async task that will gather render target data from the GPU.
    /// </summary>
    /// <param name="result">Result data</param>
    /// <returns>Download data task (not started yet)</returns>
    virtual Task* DownloadDataAsync(TextureData& result);

public:
    /// <summary>
    /// Begin task rendering.
    /// </summary>
    /// <param name="task">Active task</param>
    virtual void Begin(RenderTask* task)
    {
    }

    /// <summary>
    /// End task rendering.
    /// </summary>
    /// <param name="task">Active task</param>
    virtual void End(RenderTask* task);

    /// <summary>
    /// Present back buffer to the output.
    /// </summary>
    /// <param name="vsync">True if use vertical synchronization to lock frame rate.</param>
    virtual void Present(bool vsync);

    /// <summary>
    /// Resize output back buffer.
    /// </summary>
    /// <param name="width">New output width (in pixels).</param>
    /// <param name="height">New output height (in pixels).</param>
    /// <returns>True if cannot resize the buffers, otherwise false.</returns>
    virtual bool Resize(int32 width, int32 height) = 0;

public:
    // [GPUResource]
    String ToString() const override;
    GPUResourceType GetResourceType() const final override;
};
