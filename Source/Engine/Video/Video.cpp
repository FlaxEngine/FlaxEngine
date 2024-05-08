// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Video.h"
#include "VideoBackend.h"
#include "Engine/Audio/AudioBackend.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Scripting/Enums.h"
#if VIDEO_API_MF
#include "MF/VideoBackendMF.h"
#endif

/// <summary>
/// Video frame upload task to the GPU.
/// </summary>
class GPUUploadVideoFrameTask : public GPUTask
{
private:
    VideoBackendPlayer* _player;

public:
    GPUUploadVideoFrameTask(VideoBackendPlayer* player)
        : GPUTask(Type::UploadTexture, 0)
        , _player(player)
    {
    }

public:
    // [GPUTask]
    bool HasReference(Object* resource) const override
    {
        return _player && _player->Frame == resource;
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        if (!_player || _player->VideoFrameMemory.IsInvalid())
            return Result::MissingResources;
        GPUTexture* frame = _player->Frame;
        if (!frame->IsAllocated())
            return Result::MissingResources;
        PROFILE_CPU();
        ZoneText(_player->DebugUrl, _player->DebugUrlLen);

        if (PixelFormatExtensions::IsVideo(_player->Format))
        {
            // Allocate compressed frame uploading texture
            if (!_player->FrameUpload)
                _player->FrameUpload = GPUDevice::Instance->CreateBuffer(TEXT("VideoFrameUpload"));
            auto desc = GPUBufferDescription::Buffer(_player->VideoFrameMemory.Length(), GPUBufferFlags::ShaderResource, PixelFormat::R32_UInt, nullptr, 4, GPUResourceUsage::Dynamic);
            // TODO: add support for Transient textures (single frame data upload)
            if (_player->FrameUpload->GetDescription() != desc)
            {
                if (_player->FrameUpload->Init(desc))
                    return Result::Failed;
            }

            // Upload compressed texture data
            context->GPU->UpdateBuffer(_player->FrameUpload, _player->VideoFrameMemory.Get(), _player->VideoFrameMemory.Length());

            // Decompress data into RGBA texture
            auto cb = GPUDevice::Instance->QuadShader->GetCB(0);
            QuadShaderData cbData;
            cbData.Color = Float4((float)_player->Width, (float)_player->Height, 0, 0);
            context->GPU->UpdateCB(cb, &cbData);
            context->GPU->BindCB(0, cb);
            context->GPU->SetViewportAndScissors((float)_player->Width, (float)_player->Height);
            context->GPU->SetRenderTarget(frame->View());
            context->GPU->BindSR(0, _player->FrameUpload->View());
            ASSERT_LOW_LAYER(_player->Format == PixelFormat::YUY2);
            context->GPU->SetState(GPUDevice::Instance->GetDecodeYUY2PS());
            context->GPU->DrawFullscreenTriangle();
        }
        else
        {
            // Raw texture data upload
            uint32 rowPitch, slicePitch;
            frame->ComputePitch(0, rowPitch, slicePitch);
            context->GPU->UpdateTexture(frame, 0, 0, _player->VideoFrameMemory.Get(), rowPitch, slicePitch);
        }

        // Frame has been updated
        _player->FramesCount++;

        return Result::Ok;
    }

    void OnEnd() override
    {
        // Unlink
        if (_player && _player->UploadVideoFrameTask == this)
            _player->UploadVideoFrameTask = nullptr;
        _player = nullptr;

        GPUTask::OnEnd();
    }
};

class VideoService : public EngineService
{
public:
    VideoService()
        : EngineService(TEXT("Video"), -40)
    {
    }

    VideoBackend* Backends[4] = {};

    void InitBackend(int32 index, VideoBackend* backend)
    {
        LOG(Info, "Video initialization... (backend: {0})", backend->Base_Name());
        if (backend->Base_Init())
        {
            LOG(Warning, "Failed to initialize Video backend.");
        }
        Backends[index] = backend;
    }

    void Update() override;
    void Dispose() override;
};

VideoService VideoServiceInstance;

void VideoService::Update()
{
    PROFILE_CPU_NAMED("Video.Update");

    // Update backends
    for (VideoBackend*& backend : VideoServiceInstance.Backends)
    {
        if (backend)
            backend->Base_Update();
    }
}

void VideoService::Dispose()
{
    PROFILE_CPU_NAMED("Video.Dispose");

    // Dispose backends
    for (VideoBackend*& backend : VideoServiceInstance.Backends)
    {
        if (backend)
        {
            delete backend;
            backend = nullptr;
        }
    }
}

bool Video::CreatePlayerBackend(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player)
{
    // Pick the first backend to support the player info
    int32 index = 0;
    VideoBackend* backend;
#define TRY_USE_BACKEND(type) \
        backend = VideoServiceInstance.Backends[index]; \
        if (!backend) \
            VideoServiceInstance.InitBackend(index, backend = new type()); \
        if (!backend->Player_Create(info, player)) \
            return false;
#if VIDEO_API_MF
    TRY_USE_BACKEND(VideoBackendMF);
#endif
#undef TRY_USE_BACKEND

    LOG(Error, "Failed to setup Video playback backend for '{}'", info.Url);
    return true;
}

void VideoBackendPlayer::Created(const VideoBackendPlayerInfo& info)
{
#ifdef TRACY_ENABLE
    DebugUrlLen = info.Url.Length();
    DebugUrl = (Char*)Allocator::Allocate(DebugUrlLen * sizeof(Char) + 2);
    Platform::MemoryCopy(DebugUrl, *info.Url, DebugUrlLen * 2 + 2);
#endif
}

void VideoBackendPlayer::InitVideoFrame()
{
    if (!GPUDevice::Instance)
        return;
    if (!Frame)
        Frame = GPUDevice::Instance->CreateTexture(TEXT("VideoFrame"));
}

void VideoBackendPlayer::UpdateVideoFrame(Span<byte> data, TimeSpan time, TimeSpan duration)
{
    PROFILE_CPU();
    ZoneText(DebugUrl, DebugUrlLen);
    VideoFrameTime = time;
    VideoFrameDuration = duration;
    if (!GPUDevice::Instance || GPUDevice::Instance->GetRendererType() == RendererType::Null)
        return;

    // Ensure that sampled frame data matches the target texture size
    uint32 rowPitch, slicePitch;
    RenderTools::ComputePitch(Format, VideoFrameWidth, VideoFrameHeight, rowPitch, slicePitch);
    if (slicePitch != data.Length())
    {
        LOG(Warning, "Incorrect video frame stride {}, doesn't match stride {} of video {}x{} in format {}", data.Length(), slicePitch, Width, Height, ScriptingEnum::ToString(Format));
        return;
    }

    // Copy frame into buffer for video frames uploading
    if (VideoFrameMemory.Length() < (int32)slicePitch)
    {
        VideoFrameMemory.Allocate(slicePitch);
        if (VideoFrameMemory.IsInvalid())
        {
            OUT_OF_MEMORY;
            return;
        }
    }
    Platform::MemoryCopy(VideoFrameMemory.Get(), data.Get(), slicePitch);

    // Update output frame texture
    InitVideoFrame();
    auto desc = GPUTextureDescription::New2D(Width, Height, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget);
    if (Frame->GetDescription() != desc)
    {
        if (Frame->Init(desc))
        {
            LOG(Error, "Failed to allocate video frame texture");
            return;
        }
    }

    // Start texture upload task (if not already - only one is needed to upload the latest frame)
    if (!UploadVideoFrameTask)
    {
        UploadVideoFrameTask = New<GPUUploadVideoFrameTask>(this);
        UploadVideoFrameTask->Start();
    }
}

void VideoBackendPlayer::UpdateAudioBuffer(Span<byte> data, TimeSpan time, TimeSpan duration)
{
    PROFILE_CPU();
    ZoneText(DebugUrl, DebugUrlLen);
    AudioBufferTime = time;
    AudioBufferDuration = duration;
    if (!AudioBackend::Instance)
        return;

    // Setup audio source
    bool newSource = AudioSource == 0;
    if (newSource)
    {
        // TODO: spatial video player
        // TODO: video player volume/pan control
        AudioSource = AudioBackend::Source::Add(AudioInfo, Vector3::Zero, Quaternion::Identity, 1.0f, 1.0f, 0.0f, false, false, 1.0f, 1000.0f, 1.0f);
    }
    else
    {
        // Get the processed buffers count
        int32 numProcessedBuffers = 0;
        AudioBackend::Source::GetProcessedBuffersCount(AudioSource, numProcessedBuffers);
        if (numProcessedBuffers > 0)
        {
            // Unbind processed buffers from the source
            AudioBackend::Source::DequeueProcessedBuffers(AudioSource);
        }
    }

    // Get audio buffer
    uint32 bufferId = AudioBuffers[NextAudioBuffer];
    if (bufferId == 0)
    {
        bufferId = AudioBackend::Buffer::Create();
        AudioBuffers[NextAudioBuffer] = bufferId;
    }
    NextAudioBuffer = (NextAudioBuffer + 1) % ARRAY_COUNT(AudioBuffers);

    // Update audio buffer
    AudioDataInfo dataInfo = AudioInfo;
    const uint32 samplesPerSecond = dataInfo.SampleRate * dataInfo.NumChannels;
    const uint32 maxSamplesInData = (uint32)data.Length() * 8 / dataInfo.BitDepth;
    const uint32 maxSamplesInDuration = (uint32)Math::CeilToInt(samplesPerSecond * duration.GetTotalSeconds());
    dataInfo.NumSamples = Math::Min(maxSamplesInData, maxSamplesInDuration);
    AudioBackend::Buffer::Write(bufferId, data.Get(), dataInfo);

    // Append audio buffer
    AudioBackend::Source::QueueBuffer(AudioSource, bufferId);
    if (newSource)
    {
        AudioBackend::Source::Play(AudioSource);
    }
}

void VideoBackendPlayer::ReleaseResources()
{
    if (AudioSource)
    {
        AudioBackend::Source::Stop(AudioSource);
        AudioBackend::Source::Remove(AudioSource);
        AudioSource = 0;
    }
    for (uint32& bufferId : AudioBuffers)
    {
        if (bufferId)
        {
            AudioBackend::Buffer::Delete(bufferId);
            bufferId = 0;
        }
    }
    if (UploadVideoFrameTask)
        UploadVideoFrameTask->Cancel();
    VideoFrameMemory.Release();
    SAFE_DELETE_GPU_RESOURCE(Frame);
    SAFE_DELETE_GPU_RESOURCE(FrameUpload);
#ifdef TRACY_ENABLE
    Allocator::Free(DebugUrl);
#endif
}
