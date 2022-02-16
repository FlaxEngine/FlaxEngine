// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GPUSwapChain.h"
#include "GPUDevice.h"
#include "Textures/GPUTexture.h"
#include "Engine/Core/Log.h"

class GPUSwapChainDownloadTask : public Task
{
    friend GPUSwapChain;

public:

    GPUSwapChain* SwapChain;
    GPUTexture* Texture;

    ~GPUSwapChainDownloadTask()
    {
        if (Texture)
            Texture->DeleteObject(5);
    }

    String ToString() const override
    {
        return TEXT("GPUSwapChainDownloadTask");
    }

    bool Run() override
    {
        return false;
    }

    void Enqueue() override
    {
    }
};

Task* GPUSwapChain::DownloadDataAsync(TextureData& result)
{
    if (_downloadTask)
    {
        LOG(Warning, "Can download window backuffer data only once at the time.");
        return nullptr;
    }

    auto texture = GPUDevice::Instance->CreateTexture(String::Empty);
    if (texture->Init(GPUTextureDescription::New2D(GetWidth(), GetHeight(), GetFormat(), GPUTextureFlags::None, 1).ToStagingReadback()))
    {
        LOG(Warning, "Failed to create staging texture for the window swapchain backuffer download.");
        return nullptr;
    }

    auto task = New<GPUSwapChainDownloadTask>();
    task->SwapChain = this;
    task->Texture = texture;

    const auto downloadTask = texture->DownloadDataAsync(result);
    task->ContinueWith(downloadTask);

    _downloadTask = task;
    return task;
}

void GPUSwapChain::End(RenderTask* task)
{
    if (_downloadTask)
    {
        auto downloadTask = (GPUSwapChainDownloadTask*)_downloadTask;
        auto context = GPUDevice::Instance->GetMainContext();
        CopyBackbuffer(context, downloadTask->Texture);
        downloadTask->Execute();
        _downloadTask = nullptr;
    }
}

void GPUSwapChain::Present(bool vsync)
{
    // Check if need to show the window after the 1st paint
    if (_window->_showAfterFirstPaint)
    {
        _window->_showAfterFirstPaint = false;
        _window->Show();
    }

    // Count amount of present calls
    _presentCount++;
}
