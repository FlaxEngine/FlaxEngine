// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if VIDEO_API_MF

#include "VideoBackendMF.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/TaskGraph.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Time.h"
#include "Engine/Audio/Types.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#include <sdkddkver.h>
#if WINVER >= _WIN32_WINNT_WINBLUE && WINVER < _WIN32_WINNT_WIN10
// Fix compilation for Windows 8.1 on the latest Windows SDK
typedef enum _MFVideoSphericalFormat { } MFVideoSphericalFormat;
#endif
#if !defined(MF_SOURCE_READER_CURRENT_TYPE_INDEX) && !defined(PLATFORM_GDK)
// Fix compilation for Windows 7 on the latest Windows SDK
#define MF_SOURCE_READER_CURRENT_TYPE_INDEX 0xFFFFFFFF
#endif
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#define VIDEO_API_MF_ERROR(api, err) LOG(Warning, "[VideoBackendMF] {} failed with error 0x{:x}", TEXT(#api), (uint64)err)

struct VideoPlayerMF
{
    IMFSourceReader* SourceReader;
    uint8 Loop : 1;
    uint8 Playing : 1;
    uint8 FirstFrame : 1;
    uint8 Seek : 1;
    TimeSpan Time;
};

namespace MF
{
    Array<VideoBackendPlayer*> Players;

    bool Configure(VideoBackendPlayer& player, VideoPlayerMF& playerMF, DWORD streamIndex)
    {
        PROFILE_CPU_NAMED("Configure");
        IMFMediaType *mediaType = nullptr, *nativeType = nullptr;
        bool result = true;

        // Find the native format of the stream
        HRESULT hr = playerMF.SourceReader->GetNativeMediaType(streamIndex, MF_SOURCE_READER_CURRENT_TYPE_INDEX, &nativeType);
        if (FAILED(hr))
        {
            VIDEO_API_MF_ERROR(GetNativeMediaType, hr);
            goto END;
        }
        hr = playerMF.SourceReader->GetCurrentMediaType(streamIndex, &mediaType);
        if (FAILED(hr))
        {
            VIDEO_API_MF_ERROR(GetCurrentMediaType, hr);
            goto END;
        }
        GUID majorType, subtype;
        hr = mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        if (FAILED(hr))
        {
            VIDEO_API_MF_ERROR(GetGUID, hr);
            goto END;
        }
        hr = mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (FAILED(hr))
        {
            VIDEO_API_MF_ERROR(GetGUID, hr);
            goto END;
        }

        // Extract media information
        if (majorType == MFMediaType_Video)
        {
            UINT32 width, height;
            hr = MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);
            if (SUCCEEDED(hr))
            {
                player.Width = player.VideoFrameWidth = width;
                player.Height = player.VideoFrameHeight = height;
            }
            MFVideoArea videoArea;
            hr = mediaType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&videoArea, sizeof(MFVideoArea), NULL);
            if (SUCCEEDED(hr) && videoArea.Area.cx > 0 && videoArea.Area.cy > 0)
            {
                // Video frame has different size in memory than for display (eg. 1080p video will use 1088 height due to H264 decoding)
                player.Width = videoArea.Area.cx;
                player.Height = videoArea.Area.cy;
            }
            uint64_t fpsValue;
            hr = mediaType->GetUINT64(MF_MT_FRAME_RATE, &fpsValue);
            if (SUCCEEDED(hr))
            {
                player.FrameRate = (float)HI32(fpsValue) / (float)LO32(fpsValue);
            }
            if (subtype == MFVideoFormat_RGB32)
                player.Format = PixelFormat::B8G8R8X8_UNorm;
            else if (subtype == MFVideoFormat_ARGB32)
                player.Format = PixelFormat::B8G8R8A8_UNorm;
            else if (subtype == MFVideoFormat_RGB555)
                player.Format = PixelFormat::B5G6R5_UNorm;
            else if (subtype == MFVideoFormat_RGB555)
                player.Format = PixelFormat::B5G5R5A1_UNorm;
            else if (subtype == MFVideoFormat_NV12)
                player.Format = PixelFormat::NV12;
            else if (subtype == MFVideoFormat_YUY2)
                player.Format = PixelFormat::YUY2;
#if (WDK_NTDDI_VERSION >= NTDDI_WIN10)
            else if (subtype == MFVideoFormat_A2R10G10B10)
                player.Format = PixelFormat::R10G10B10A2_UNorm;
            else if (subtype == MFVideoFormat_A16B16G16R16F)
                player.Format = PixelFormat::R16G16B16A16_Float;
#endif
            else
            {
                // Reconfigure decoder to output supported format by force
                IMFMediaType* customType = nullptr;
                hr = MFCreateMediaType(&customType);
                if (FAILED(hr))
                {
                    VIDEO_API_MF_ERROR(MFCreateMediaType, hr);
                    goto END;
                }
                customType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
                customType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
                MFSetAttributeSize(customType, MF_MT_FRAME_SIZE, width, height);
                hr = playerMF.SourceReader->SetCurrentMediaType(streamIndex, nullptr, customType);
                if (FAILED(hr))
                {
                    VIDEO_API_MF_ERROR(SetCurrentMediaType, hr);
                    goto END;
                }
                player.Format = PixelFormat::YUY2;
                customType->Release();
            }
        }
        else if (majorType == MFMediaType_Audio)
        {
            player.AudioInfo.SampleRate = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
            player.AudioInfo.NumChannels = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
            player.AudioInfo.BitDepth = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
            if (subtype != MFAudioFormat_PCM)
            {
                // Reconfigure decoder to output audio data in PCM format
                IMFMediaType* customType = nullptr;
                hr = MFCreateMediaType(&customType);
                if (FAILED(hr))
                {
                    VIDEO_API_MF_ERROR(MFCreateMediaType, hr);
                    goto END;
                }
                customType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
                customType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
                hr = playerMF.SourceReader->SetCurrentMediaType(streamIndex, nullptr, customType);
                if (FAILED(hr))
                {
                    VIDEO_API_MF_ERROR(SetCurrentMediaType, hr);
                    goto END;
                }
                customType->Release();
            }
        }

        result = false;
    END:
        SAFE_RELEASE(mediaType);
        return result;
    }

    bool ReadStream(VideoBackendPlayer& player, VideoPlayerMF& playerMF, DWORD streamIndex, TimeSpan dt)
    {
        PROFILE_CPU_NAMED("ReadStream");
        ZoneText(player.DebugUrl, player.DebugUrlLen);
        const bool isVideo = streamIndex == MF_SOURCE_READER_FIRST_VIDEO_STREAM;
        const bool isAudio = streamIndex == MF_SOURCE_READER_FIRST_AUDIO_STREAM;
        int32 goodSamples = 1;
        TimeSpan validTimeRangeStart(0), validTimeRangeEnd(0);
        if (isAudio)
        {
            constexpr int32 AudioFramesQueue = 10; // How many frames to read into the audio buffers queue in advance (to improve audio playback smoothness)
            if (player.AudioBufferDuration.Ticks == 0)
            {
                // Read more samples for audio to enqueue multiple audio buffers for smoother playback
                goodSamples = AudioFramesQueue;
            }
            else
            {
                // Skip reading if the last sample was already over this range (we've got enough in a queue)
                validTimeRangeStart = player.AudioBufferTime - player.AudioBufferDuration * AudioFramesQueue;
                validTimeRangeEnd = validTimeRangeStart + player.AudioBufferDuration;
                if (Math::IsInRange(playerMF.Time, validTimeRangeStart, validTimeRangeEnd))
                {
                    return false;
                }

                // Allow to read future samples within queue range
                validTimeRangeStart = player.AudioBufferTime - player.AudioBufferDuration;
                validTimeRangeEnd = player.AudioBufferTime + player.AudioBufferDuration * AudioFramesQueue;

                // Read more samples to keep queue at capacity
                TimeSpan targetQueueEnd = playerMF.Time + player.AudioBufferDuration * AudioFramesQueue;
                TimeSpan activeBufferEnd = player.AudioBufferTime + player.AudioBufferDuration;
                TimeSpan missingQueueDuration = targetQueueEnd - activeBufferEnd;
                goodSamples = (int32)Math::DivideAndRoundUp(missingQueueDuration.Ticks, player.AudioBufferDuration.Ticks);
                if (goodSamples < 1)
                    goodSamples = 1;
            }
        }
        else if (isVideo)
        {
            // Check if the current frame is valid (eg. when playing 24fps video at 60fps)
            if (player.VideoFrameDuration.Ticks > 0 &&
                Math::IsInRange(playerMF.Time, player.VideoFrameTime, player.VideoFrameTime + player.VideoFrameDuration))
            {
                return false;
            }
        }

        // Read samples until frame is matching the current time
        int32 samplesLeft = 500;
        int32 goodSamplesLeft = goodSamples;
        HRESULT hr;
        for (; samplesLeft > 0 && goodSamplesLeft > 0; samplesLeft--)
        {
            // Read sample
            DWORD flags = 0;
            LONGLONG samplePos = 0, sampleDuration = 0;
            IMFSample* sample = nullptr;
            {
                PROFILE_CPU_NAMED("ReadSample");
                hr = playerMF.SourceReader->ReadSample(streamIndex, 0, &streamIndex, &flags, &samplePos, &sample);
                if (FAILED(hr))
                {
                    VIDEO_API_MF_ERROR(ReadSample, hr);
                    break;
                }
            }
            TimeSpan frameTime((int64)samplePos);
            TimeSpan franeDuration = player.FrameRate > 0 ? TimeSpan::FromSeconds(1.0 / player.FrameRate) : dt;
            if (sample && sample->GetSampleDuration(&sampleDuration) == S_OK && sampleDuration > 0)
            {
                franeDuration.Ticks = sampleDuration;
            }
            //const int32 framesToTime = (playerMF.Time.Ticks - frameTime.Ticks) / franeDuration.Ticks;
            bool isGoodSample = goodSamples != goodSamplesLeft; // If we've reached good frame, then use following frames too
            if (validTimeRangeStart.Ticks != 0)
                isGoodSample |= Math::IsInRange(frameTime, validTimeRangeStart, validTimeRangeEnd); // Ensure frame hits the valid range
            else
                isGoodSample |= Math::IsInRange(playerMF.Time, frameTime, frameTime + franeDuration); // Ensure current time hits this frame range

            // Process sample
            if (sample && isGoodSample)
            {
                PROFILE_CPU_NAMED("ProcessSample");

                // Lock sample buffer memory (try to use 2D buffer for more direct memory access)
                IMFMediaBuffer* buffer = nullptr;
                IMF2DBuffer* buffer2D = nullptr;
                BYTE* bufferData = nullptr;
                LONG bufferStride = 0;
                if (isVideo && player.Format != PixelFormat::NV12 && sample->GetBufferByIndex(0, &buffer) == S_OK && buffer->QueryInterface(IID_PPV_ARGS(&buffer2D)) == S_OK)
                {
                    LONG bufferPitch = 0;
                    hr = buffer2D->Lock2D(&bufferData, &bufferPitch);
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(GetCurrentLength, hr);
                        goto PROCESS_SAMPLE_END;
                    }
                    if (bufferPitch < 0)
                        bufferPitch = -bufferPitch; // Flipped image
                    bufferStride = bufferPitch * player.VideoFrameHeight;
                }
                else
                {
                    if (buffer)
                    {
                        buffer->Release();
                        buffer = nullptr;
                    }
                    DWORD bufferLength;
                    hr = sample->ConvertToContiguousBuffer(&buffer);
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(ConvertToContiguousBuffer, hr);
                        goto PROCESS_SAMPLE_END;
                    }
                    hr = buffer->GetCurrentLength(&bufferLength);
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(GetCurrentLength, hr);
                        goto PROCESS_SAMPLE_END;
                    }
                    DWORD bufferMaxLen = 0, bufferCurrentLength = 0;
                    hr = buffer->Lock(&bufferData, &bufferMaxLen, &bufferCurrentLength);
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(Lock, hr);
                        goto PROCESS_SAMPLE_END;
                    }
                    bufferStride = bufferCurrentLength;
                }

                if (isVideo)
                {
                    // Send pixels to the texture
                    player.UpdateVideoFrame(Span<byte>(bufferData, bufferStride), frameTime, franeDuration);
                }
                else if (isAudio)
                {
                    // Send PCM data
                    player.UpdateAudioBuffer(Span<byte>(bufferData, bufferStride), frameTime, franeDuration);
                }

                // Unlock sample buffer memory
                if (buffer2D)
                {
                    hr = buffer2D->Unlock2D();
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(Unlock2D, hr);
                    }
                }
                else
                {
                    hr = buffer->Unlock();
                    if (FAILED(hr))
                    {
                        VIDEO_API_MF_ERROR(Unlock, hr);
                    }
                }

            PROCESS_SAMPLE_END:
                if (buffer2D)
                    buffer2D->Release();
                buffer->Release();
            }
            if (sample)
                sample->Release();
            if (isGoodSample)
            {
                goodSamplesLeft--;
                if (!sample)
                    return true; // Got good sample but without data so seek
            }

            if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                // Media ended
                break;
            }
            if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED || flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
            {
                // Format/metadata might have changed so update the stream
                Configure(player, playerMF, streamIndex);
            }
        }

        // True if run out of samples and failed to get frame for the current time
        return samplesLeft == 0;
    }

    void UpdatePlayer(int32 index)
    {
        PROFILE_CPU();
        auto& player = *Players[index];
        ZoneText(player.DebugUrl, player.DebugUrlLen);
        auto& playerMF = player.GetBackendState<VideoPlayerMF>();

        // Skip paused player
        if (!playerMF.Playing && !playerMF.Seek)
            return;

        bool useTimeScale = true;
#if USE_EDITOR
        if (!Editor::IsPlayMode)
            useTimeScale = false;
#endif
        TimeSpan dt = useTimeScale ? Time::Update.DeltaTime : Time::Update.UnscaledDeltaTime;

        // Update playback time
        if (playerMF.FirstFrame)
        {
            playerMF.FirstFrame = 0;
            playerMF.Seek = 1;
        }
        else if (playerMF.Playing)
        {
            playerMF.Time += dt;
        }
        if (playerMF.Time > player.Duration)
        {
            if (playerMF.Loop)
            {
                // Loop
                playerMF.Time.Ticks %= player.Duration.Ticks;
                playerMF.Seek = 1;
                player.PlayAudio();
            }
            else
            {
                // End
                playerMF.Time = player.Duration;
            }
        }

        // Update current position
        bool canSeek = true;
    SEEK_START:
        if (playerMF.Seek)
        {
            // Reset cached frames timings
            player.VideoFrameDuration = player.AudioBufferDuration = TimeSpan::Zero();

            playerMF.Seek = 0;
            PROPVARIANT var;
            PropVariantInit(&var);
            var.vt = VT_I8;
            var.hVal.QuadPart = playerMF.Time.Ticks;
            PROFILE_CPU_NAMED("SetCurrentPosition");
            playerMF.SourceReader->SetCurrentPosition(GUID_NULL, var);

            // Note:
            // SetCurrentPosition method does not guarantee exact seeking.
            // The accuracy of the seek depends on the media content.
            // If the media content contains a video stream, the SetCurrentPosition method typically seeks to the nearest key frame before the desired position.
            // After seeking, the application should call ReadSample and advance to the desired position.
        }

        // Update streams
        if (ReadStream(player, playerMF, MF_SOURCE_READER_FIRST_VIDEO_STREAM, dt))
        {
            // Failed to pick a valid sample so try again with seeking
            playerMF.Seek = 1;
            if (canSeek)
            {
                // Prevent deadlock on sync
                canSeek = false;
                goto SEEK_START;
            }
        }
        if (player.AudioInfo.BitDepth != 0)
            ReadStream(player, playerMF, MF_SOURCE_READER_FIRST_AUDIO_STREAM, dt);

        player.Tick();
    }
}

bool VideoBackendMF::Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player = VideoBackendPlayer();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();

    // Load media
    IMFAttributes* attributes = nullptr;
    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr))
    {
        VIDEO_API_MF_ERROR(MFCreateAttributes, hr);
        return true;
    }
    attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 1);
    attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, 1);
    IMFSourceReader* sourceReader = nullptr;
    hr = MFCreateSourceReaderFromURL(*info.Url, attributes, &sourceReader);
    attributes->Release();
    if (FAILED(hr) || !sourceReader)
    {
        VIDEO_API_MF_ERROR(MFCreateSourceReaderFromURL, hr);
        return true;
    }
    sourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 1);
    bool hasAudio = sourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 1) == S_OK;
    playerMF.SourceReader = sourceReader;

    // Read media info
    if (MF::Configure(player, playerMF, MF_SOURCE_READER_FIRST_VIDEO_STREAM) ||
        hasAudio && MF::Configure(player, playerMF, MF_SOURCE_READER_FIRST_AUDIO_STREAM))
        return true;
    PROPVARIANT var;
    hr = sourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (SUCCEEDED(hr))
    {
        player.Duration.Ticks = var.vt == VT_UI8 ? var.uhVal.QuadPart : 0;
        PropVariantClear(&var);
    }

    // Setup player data
    player.Backend = this;
    playerMF.Loop = info.Loop;
    playerMF.FirstFrame = 1;
    player.Created(info);
    MF::Players.Add(&player);

    return false;
}

void VideoBackendMF::Player_Destroy(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player.ReleaseResources();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    playerMF.SourceReader->Release();
    MF::Players.Remove(&player);
    player = VideoBackendPlayer();
}

void VideoBackendMF::Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    playerMF.Loop = info.Loop;
    player.Updated(info);
}

void VideoBackendMF::Player_Play(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    playerMF.Playing = 1;
    player.PlayAudio();
}

void VideoBackendMF::Player_Pause(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    playerMF.Playing = 0;
    player.PauseAudio();
}

void VideoBackendMF::Player_Stop(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    playerMF.Time = TimeSpan::Zero();
    playerMF.Playing = 0;
    playerMF.FirstFrame = 1;
    playerMF.Seek = 1;
    player.StopAudio();
}

void VideoBackendMF::Player_Seek(VideoBackendPlayer& player, TimeSpan time)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    if (playerMF.Time != time)
    {
        playerMF.Time = time;
        playerMF.Seek = 1;
        player.StopAudio();
    }
}

TimeSpan VideoBackendMF::Player_GetTime(const VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMF = player.GetBackendState<VideoPlayerMF>();
    return playerMF.Time;
}

const Char* VideoBackendMF::Base_Name()
{
    return TEXT("Media Foundation");
}

bool VideoBackendMF::Base_Init()
{
    PROFILE_CPU();

    // Init COM
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != 0x80010106) // 0x80010106 = Cannot change thread mode after it is set.
    {
        VIDEO_API_MF_ERROR(CoInitializeEx, hr);
        return true;
    }

    // Init Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        VIDEO_API_MF_ERROR(MFStartup, hr);
        return true;
    }

    return false;
}

void VideoBackendMF::Base_Update(TaskGraph* graph)
{
    // Schedule work to update all videos in async
    Function<void(int32)> job;
    job.Bind(MF::UpdatePlayer);
    graph->DispatchJob(job, MF::Players.Count());
}

void VideoBackendMF::Base_Dispose()
{
    PROFILE_CPU();

    // Shutdown
    MFShutdown();
}

#endif
