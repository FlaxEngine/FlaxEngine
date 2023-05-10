// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Screenshot.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/GPUResourceProperty.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Engine/Globals.h"
#if COMPILE_WITH_TEXTURE_TOOL
#include "Engine/Tools/TextureTool/TextureTool.h"
#endif

/// <summary>
/// Capture screenshot helper
/// </summary>
/// <seealso cref="ThreadPoolTask" />
class CaptureScreenshot : public ThreadPoolTask
{
    friend Screenshot;
private:
    TextureData _data;
    GPUTextureReference _texture;
    ScriptingObjectReference<RenderTask> _renderTask;
    String _path;
    DateTime _startTime;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CaptureScreenshot"/> class.
    /// </summary>
    /// <param name="target">The target.</param>
    /// <param name="path">The path.</param>
    CaptureScreenshot(GPUTexture* target, const StringView& path)
        : _texture(target)
        , _path(path)
        , _startTime(DateTime::NowUTC())
    {
        ASSERT(_texture != nullptr);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="CaptureScreenshot"/> class.
    /// </summary>
    /// <param name="target">The target.</param>
    /// <param name="path">The path.</param>
    CaptureScreenshot(RenderTask* target, const StringView& path)
        : _renderTask(target)
        , _path(path)
        , _startTime(DateTime::NowUTC())
    {
        ASSERT(_renderTask != nullptr);
    }

public:
    /// <summary>
    /// Gets the texture data container.
    /// </summary>
    /// <returns>Texture data</returns>
    FORCE_INLINE TextureData& GetData()
    {
        return _data;
    }

protected:
    // [ThreadPoolTask]
    bool Run() override;
    void OnFail() override;
};

bool CaptureScreenshot::Run()
{
    const auto texture = _texture.Get();
    if (texture == nullptr && _renderTask == nullptr)
    {
        LOG(Warning, "Missing target render task.");
        return true;
    }

    // TODO: how about a case two or more screenshots at the same second? update counter and check files

    // Ensure that path is valid and folder exists
    String screenshotsDir;
    if (_path.IsEmpty())
    {
#if USE_EDITOR
        screenshotsDir = Globals::ProjectFolder / TEXT("Screenshots");
#else
        screenshotsDir = Globals::ProductLocalFolder / TEXT("Screenshots");
#endif
        _path = screenshotsDir / String::Format(TEXT("Screenshot_{0}.png"), DateTime::Now().ToFileNameString());
    }
    else
    {
        screenshotsDir = StringUtils::GetDirectoryName(_path);
    }
    if (!FileSystem::DirectoryExists(screenshotsDir))
        FileSystem::CreateDirectory(screenshotsDir);

#if COMPILE_WITH_TEXTURE_TOOL
    // Export to file
    if (TextureTool::ExportTexture(_path, _data))
    {
        LOG(Warning, "Cannot export screenshot to file.");
        return true;
    }
#else
		LOG(Warning, "Cannot export screenshot to file. No textures exporting support in build.");
		return true;
#endif

    LOG(Info, "Saved screenshot '{1}' (time: {0} ms)", Math::RoundToInt((float)(DateTime::NowUTC() - _startTime).GetTotalMilliseconds()), _path);

    _renderTask = nullptr;
    return false;
}

void CaptureScreenshot::OnFail()
{
    LOG(Warning, "Cannot take screenshot.");

    // Base
    ThreadPoolTask::OnFail();
}

Delegate<Color32> Screenshot::PixelReadDelegate;

/// <summary>
/// Capture screenshot helper
/// </summary>
/// <seealso cref="ThreadPoolTask" />
class GetPixelData : public ThreadPoolTask
{
    friend Screenshot;
private:
    TextureData _data;
    Color32 _color;

public:
    int32 x;
    int32 y;

public:
    /// <summary>
    /// Gets the texture data container.
    /// </summary>
    /// <returns>Texture data</returns>
    FORCE_INLINE TextureData& GetData()
    {
        return _data;
    }

    FORCE_INLINE Color32 GetColor()
    {
        return _color;
    }

protected:
    // [ThreadPoolTask]
    bool Run() override;
    void OnFail() override;
};

bool GetPixelData::Run()
{
    LOG(Warning, "REAL");
    TextureMipData *mipData = _data.GetData(0, 0);
    Array<Color32> pixels;
    mipData->GetPixels(pixels, _data.Width, _data.Height, _data.Format);
    
    LOG(Warning, "{0}, {1} ({2} at {3})", x, y, pixels.Count(), (y * _data.Width) + x);
    _color = pixels[(y * _data.Width) + x];
    LOG(Warning, "really real");
    LOG(Warning, "Color: R: {0}, G: {1}, B: {2}", _color.R, _color.G, _color.B);

    LOG(Warning, "Bound functions: {0}", Screenshot::PixelReadDelegate.Count());
    Screenshot::PixelReadDelegate(_color);
    return false;
}

void GetPixelData::OnFail()
{
    LOG(Warning, "Cannot get pixel data.");

    // Base
    ThreadPoolTask::OnFail();
}


void Screenshot::Capture(GPUTexture* target, const StringView& path)
{
    // Validate
    if (target == nullptr)
    {
        LOG(Warning, "Cannot take screenshot. Render target texture is not allocated.");
        return;
    }
    if (target->Depth() != 1)
    {
        LOG(Warning, "Cannot take screenshot. 3D textures are not supported.");
        return;
    }
    if (GPUDevice::Instance == nullptr || GPUDevice::Instance->GetState() != GPUDevice::DeviceState::Ready)
    {
        LOG(Warning, "Cannot take screenshot. Graphics device is not ready.");
        return;
    }

    // Faster path for staging textures that contents are ready to access on a CPU
    if (target->IsStaging())
    {
        CaptureScreenshot screenshot(target, path);
        TextureData& data = screenshot.GetData();
        data.Width = target->Width();
        data.Height = target->Height();
        data.Format = target->Format();
        data.Depth = target->Depth();
        data.Items.Resize(target->ArraySize());
        for (int32 arrayIndex = 0; arrayIndex < target->ArraySize(); arrayIndex++)
        {
            auto& slice = data.Items[arrayIndex];
            slice.Mips.Resize(target->MipLevels());
            for (int32 mipIndex = 0; mipIndex < target->MipLevels(); mipIndex++)
            {
                auto& mip = slice.Mips[mipIndex];
                if (target->GetData(arrayIndex, mipIndex, mip))
                {
                    LOG(Warning, "Cannot take screenshot. Failed to get texture data.");
                    return;
                }
            }
        }
        screenshot.Run();
        return;
    }

    // Create tasks
    auto saveTask = New<CaptureScreenshot>(target, path);
    auto downloadTask = target->DownloadDataAsync(saveTask->GetData());
    if (downloadTask == nullptr)
    {
        LOG(Warning, "Cannot capture screenshot. Cannot create download async task.");
        Delete(saveTask);
        return;
    }

    // Start
    downloadTask->ContinueWith(saveTask);
    downloadTask->Start();
}

void Screenshot::Capture(SceneRenderTask* target, const StringView& path)
{
    // Select default task if none provided
    if (target == nullptr)
    {
        Capture(path);
        return;
    }

    // Validate
    if ((target->Output == nullptr && target->SwapChain == nullptr) || (target->Output && !target->Output->IsAllocated()))
    {
        LOG(Warning, "Cannot take screenshot. Render task output is not allocated.");
        return;
    }
    if (GPUDevice::Instance == nullptr || GPUDevice::Instance->GetState() != GPUDevice::DeviceState::Ready)
    {
        LOG(Warning, "Cannot take screenshot. Graphics device is not ready.");
        return;
    }

    // Create tasks
    auto saveTask = New<CaptureScreenshot>(target, path);
    Task* downloadTask;
    if (target->Output)
        downloadTask = target->Output->DownloadDataAsync(saveTask->GetData());
    else
        downloadTask = target->SwapChain->DownloadDataAsync(saveTask->GetData());
    if (downloadTask == nullptr)
    {
        LOG(Warning, "Cannot capture screenshot. Cannot create download async task.");
        Delete(saveTask);
        return;
    }

    // Start
    downloadTask->ContinueWith(saveTask);
    downloadTask->Start();
}

void Screenshot::Capture(const StringView& path)
{
    const auto mainTask = MainRenderTask::Instance;
    if (!mainTask)
    {
        LOG(Warning, "Cannot take screenshot. Missing main rendering task.");
        return;
    }

    Capture(mainTask, path);
}

Color32 Screenshot::GetPixelAt(int32 x, int32 y) {
    GPUSwapChain* swapChain = Engine::MainWindow->GetSwapChain();

    auto getPixelTask = New<GetPixelData>();
    getPixelTask->x = x;
    getPixelTask->y = y;

    Task* downloadTask = swapChain->DownloadDataAsync(getPixelTask->GetData());
    downloadTask->ContinueWith(getPixelTask);
    LOG(Warning, "Started download task. real");
    downloadTask->Start();

    return getPixelTask->GetColor();
}
