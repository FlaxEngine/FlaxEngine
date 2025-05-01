// Copyright (c) Wojciech Figat. All rights reserved.

#if VIDEO_API_ANDROID

#include "VideoBackendAndroid.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Threading/TaskGraph.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Audio/Types.h"
#include "Engine/Utilities/StringConverter.h"
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#define VIDEO_API_ANDROID_DEBUG (0)
#define VIDEO_API_ANDROID_ERROR(api, err) LOG(Warning, "[VideoBackendAndroid] {} failed with error {}", TEXT(#api), (int64)err)

struct VideoPlayerAndroid
{
    AMediaExtractor* Extractor;
    AMediaCodec* VideoCodec;
    AMediaCodec* AudioCodec;
    AMediaFormat* VideoFormat;
    AMediaFormat* AudioFormat;
    uint8 Loop : 1;
    uint8 Playing : 1;
    uint8 InputEnded : 1;
    uint8 OutputEnded : 1;
    uint16 VideoStride;
    uint16 VideoTrackIndex;
    uint16 AudioTrackIndex;
};

namespace Android
{
    Array<VideoBackendPlayer*> Players;

    // Reference: http://developer.android.com/reference/android/media/MediaCodecInfo.CodecCapabilities.html
    enum ColourFormat
    {
        COLOR_Format12bitRGB444 = 3,
        COLOR_Format16bitARGB1555 = 5,
        COLOR_Format16bitARGB4444 = 4,
        COLOR_Format16bitBGR565 = 7,
        COLOR_Format16bitRGB565 = 6,
        COLOR_Format18BitBGR666 = 41,
        COLOR_Format18bitARGB1665 = 9,
        COLOR_Format18bitRGB666 = 8,
        COLOR_Format19bitARGB1666 = 10,
        COLOR_Format24BitABGR6666 = 43,
        COLOR_Format24BitARGB6666 = 42,
        COLOR_Format24bitARGB1887 = 13,
        COLOR_Format24bitBGR888 = 12,
        COLOR_Format24bitRGB888 = 11,
        COLOR_Format32bitABGR8888 = 0x7f00a000,
        COLOR_Format32bitARGB8888 = 16,
        COLOR_Format32bitBGRA8888 = 15,
        COLOR_Format8bitRGB332 = 2,
        COLOR_FormatCbYCrY = 27,
        COLOR_FormatCrYCbY = 28,
        COLOR_FormatL16 = 36,
        COLOR_FormatL2 = 33,
        COLOR_FormatL32 = 38,
        COLOR_FormatL4 = 34,
        COLOR_FormatL8 = 35,
        COLOR_FormatMonochrome = 1,
        COLOR_FormatRGBAFlexible = 0x7f36a888,
        COLOR_FormatRGBFlexible = 0x7f36b888,
        COLOR_FormatRawBayer10bit = 31,
        COLOR_FormatRawBayer8bit = 30,
        COLOR_FormatRawBayer8bitcompressed = 32,
        COLOR_FormatSurface = 0x7f000789,
        COLOR_FormatYCbYCr = 25,
        COLOR_FormatYCrYCb = 26,
        COLOR_FormatYUV411PackedPlanar = 18,
        COLOR_FormatYUV411Planar = 17,
        COLOR_FormatYUV420Flexible = 0x7f420888,
        COLOR_FormatYUV420PackedPlanar = 20,
        COLOR_FormatYUV420PackedSemiPlanar = 39,
        COLOR_FormatYUV420Planar = 19,
        COLOR_FormatYUV420SemiPlanar = 21,
        COLOR_FormatYUV422Flexible = 0x7f422888,
        COLOR_FormatYUV422PackedPlanar = 23,
        COLOR_FormatYUV422PackedSemiPlanar = 40,
        COLOR_FormatYUV422Planar = 22,
        COLOR_FormatYUV422SemiPlanar = 24,
        COLOR_FormatYUV444Flexible = 0x7f444888,
        COLOR_FormatYUV444Interleaved = 29,
        COLOR_QCOM_FormatYUV420SemiPlanar = 0x7fa30c00,
        COLOR_TI_FormatYUV420PackedSemiPlanar = 0x7f000100,
    };

    ssize_t AMediaDataSourceReadAt(void* userdata, off64_t offset, void* buffer, size_t size)
    {
        if (size == 0)
            return 0;
        auto* stream = (FileReadStream*)userdata;
        stream->SetPosition((uint32)offset);
        stream->ReadBytes(buffer, size);
        return (ssize_t)size;
    }

    ssize_t AMediaDataSourceGetSize(void* userdata)
    {
        auto* stream = (FileReadStream*)userdata;
        return (ssize_t)stream->GetLength();
    }

    void AMediaDataSourceClose(void* userdata)
    {
        auto* stream = (FileReadStream*)userdata;
        Delete(stream);
    }

    void UpdateFormat(VideoBackendPlayer& player, VideoPlayerAndroid& playerAndroid, AMediaCodec* codec, AMediaFormat* format)
    {
        const bool isVideo = codec == playerAndroid.VideoCodec;
        const bool isAudio = codec == playerAndroid.AudioCodec;
        if (isVideo)
        {
            int32_t frameWidth = 0, frameHeight = 0, frameRate = 0, colorFormat = 0, stride = 0;
            float frameRateF = 0.0f;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &frameWidth);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &frameHeight);
            if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate) && frameRate > 0)
                player.FrameRate = (float)frameRate;
            else if (AMediaFormat_getFloat(format, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRateF) && frameRateF > 0)
                player.FrameRate = frameRateF;
            else
                player.FrameRate = 60;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_STRIDE, &stride);
            playerAndroid.VideoStride = stride;
            player.Width = player.VideoFrameWidth = frameWidth;
            player.Height = player.VideoFrameHeight = frameHeight;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat);
            switch (colorFormat)
            {
            case COLOR_Format32bitABGR8888:
                player.Format = PixelFormat::R8G8B8A8_UNorm;
                break;
            case COLOR_Format32bitBGRA8888:
                player.Format = PixelFormat::B8G8R8A8_UNorm;
                break;
            case COLOR_FormatYUV420SemiPlanar:
                player.Format = PixelFormat::NV12;
                break;
            case COLOR_FormatYUV422SemiPlanar:
                player.Format = PixelFormat::YUY2;
                break;
            default:
                player.Format = PixelFormat::Unknown;
                LOG(Error, "[VideoBackendAndroid] Unsupported video color format {}", colorFormat);
                break;
            }
#if VIDEO_API_ANDROID_DEBUG
            LOG(Info, "[VideoBackendAndroid] Video track: {}x{}, {}fps", player.Width, player.Height, player.FrameRate);
#endif
        }
        else if (isAudio)
        {
            int32_t sampleRate = 0, channelCount = 0, bitsPerSample = 0;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount);
            player.AudioInfo.SampleRate = sampleRate;
            player.AudioInfo.NumChannels = channelCount;
            if (AMediaFormat_getInt32(format, "bits-per-sample", &bitsPerSample) && bitsPerSample > 0)
                player.AudioInfo.BitDepth = bitsPerSample;
            else
                player.AudioInfo.BitDepth = 16;
#if VIDEO_API_ANDROID_DEBUG
            LOG(Info, "[VideoBackendAndroid] Audio track: {} channels, {} bits, {} kHz sample rate", player.AudioInfo.NumChannels, player.AudioInfo.BitDepth, player.AudioInfo.SampleRate / 1000);
#endif
        }
    }

    void ReadCodecOutput(VideoBackendPlayer& player, VideoPlayerAndroid& playerAndroid, AMediaCodec* codec, AMediaFormat* format)
    {
        if (!codec)
            return;
        PROFILE_CPU();
        AMediaCodecBufferInfo bufferInfo;
        ssize_t bufferIndex = AMediaCodec_dequeueOutputBuffer(codec, &bufferInfo, 0);
        if (bufferIndex >= 0)
        {
            if (bufferInfo.size > 0)
            {
                TimeSpan frameTime(bufferInfo.presentationTimeUs * 10), frameDuration = TimeSpan::FromSeconds(1.0f / player.FrameRate);
                size_t bufferSize = 0;
                uint8_t* buffer = AMediaCodec_getOutputBuffer(codec, bufferIndex, &bufferSize);
                ASSERT(buffer && bufferSize);
                if (codec == playerAndroid.VideoCodec)
                {
                    // Depending on the format vide frame might have different dimensions (eg. h264 block padding) so deduct this from buffer size (Android doesn't report correct frame cropping)
                    switch (player.Format)
                    {
                    case PixelFormat::YUY2:
                    case PixelFormat::NV12:
                        //player.VideoFrameHeight = bufferSize / playerAndroid.VideoStride / 3 * 2;
                        bufferSize = player.VideoFrameHeight * playerAndroid.VideoStride * 3 / 2;
                        break;
                    }

                    // TODO: use VideoStride and repack texture if stride is different from RenderTools::ComputePitch (UpdateVideoFrame can handle pitch convert directly into frame buffer)
                    player.UpdateVideoFrame(Span<byte>(buffer, bufferSize), frameTime, frameDuration);
                }
                else if (codec == playerAndroid.AudioCodec)
                {
                    player.UpdateAudioBuffer(Span<byte>(buffer + bufferInfo.offset, bufferInfo.size), frameTime, frameDuration);
                }
            }
            media_status_t status = AMediaCodec_releaseOutputBuffer(codec, bufferIndex, false);
            if (status != AMEDIA_OK)
            {
                VIDEO_API_ANDROID_ERROR(AMediaCodec_releaseOutputBuffer, status);
            }
        }
        else if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            // Skip
        }
        else if (bufferIndex == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
        {
            // Ignore
        }
        else if (bufferIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            if (format)
            {
                AMediaFormat_delete(format);
                format = nullptr;
            }
            format = AMediaCodec_getOutputFormat(codec);
            ASSERT_LOW_LAYER(format);
            UpdateFormat(player, playerAndroid, codec, format);
        }
        else
        {
            VIDEO_API_ANDROID_ERROR(AMediaCodec_dequeueOutputBuffer, bufferIndex);
        }
    }

    void UpdatePlayer(int32 index)
    {
        PROFILE_CPU();
        auto& player = *Players[index];
        ZoneText(player.DebugUrl, player.DebugUrlLen);
        auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();

        // Skip paused player
        if (!playerAndroid.Playing || (playerAndroid.InputEnded && playerAndroid.OutputEnded))
            return;
        media_status_t status;
        ssize_t bufferIndex;

        // Get current sample info
        int64_t presentationTimeUs = AMediaExtractor_getSampleTime(playerAndroid.Extractor);
        int trackIndex = AMediaExtractor_getSampleTrackIndex(playerAndroid.Extractor);
        if (trackIndex < 0)
        {
#if VIDEO_API_ANDROID_DEBUG
            LOG(Info, "[VideoBackendAndroid] Samples track ended");
#endif
            if (playerAndroid.Loop)
            {
                // Loop
                status = AMediaExtractor_seekTo(playerAndroid.Extractor, 0, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
                if (status != AMEDIA_OK)
                {
                    VIDEO_API_ANDROID_ERROR(AMediaExtractor_seekTo, status);
                }
                if (playerAndroid.VideoCodec)
                    AMediaCodec_flush(playerAndroid.VideoCodec);
                if (playerAndroid.AudioCodec)
                    AMediaCodec_flush(playerAndroid.VideoCodec);
            }
            else
            {
                // Emd
                playerAndroid.InputEnded = playerAndroid.OutputEnded = 1;
            }
        }
        else if (trackIndex == playerAndroid.VideoTrackIndex || trackIndex == playerAndroid.AudioTrackIndex)
        {
            auto codec = trackIndex == playerAndroid.VideoTrackIndex ? playerAndroid.VideoCodec : playerAndroid.AudioCodec;

            // Process input buffer
            bufferIndex = AMediaCodec_dequeueInputBuffer(codec, 2000);
            if (bufferIndex >= 0)
            {
                size_t bufferSize;
                uint8_t* buffer = AMediaCodec_getInputBuffer(codec, bufferIndex, &bufferSize);
                ssize_t sampleSize = AMediaExtractor_readSampleData(playerAndroid.Extractor, buffer, bufferSize);
                uint32_t queueInputFlags = 0;
                if (sampleSize < 0)
                {
                    queueInputFlags |= AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM;
                    sampleSize = 0;
                }
                status = AMediaCodec_queueInputBuffer(codec, bufferIndex, 0, sampleSize, presentationTimeUs, queueInputFlags);
                if (status != AMEDIA_OK)
                {
                    VIDEO_API_ANDROID_ERROR(AMediaCodec_queueInputBuffer, status);
                }
                AMediaExtractor_advance(playerAndroid.Extractor);
            }
            else if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
            {
                // Skip
            }
            else
            {
                VIDEO_API_ANDROID_ERROR(AMediaCodec_dequeueInputBuffer, bufferIndex);
            }
        }

        if (!playerAndroid.OutputEnded)
        {
            // Process output buffers
            ReadCodecOutput(player, playerAndroid, playerAndroid.VideoCodec, playerAndroid.VideoFormat);
            ReadCodecOutput(player, playerAndroid, playerAndroid.AudioCodec, playerAndroid.AudioFormat);
        }

        player.Tick();
    }
}

bool VideoBackendAndroid::Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player = VideoBackendPlayer();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    media_status_t status;

    // Load media
    playerAndroid.Extractor = AMediaExtractor_new();
    if (!playerAndroid.Extractor)
    {
        VIDEO_API_ANDROID_ERROR(AMediaExtractor_new, 0);
        return true;
    }
    FileReadStream* fileStream = nullptr;
    if (!info.Url.StartsWith(TEXT("http"), StringSearchCase::IgnoreCase))
    {
        if (info.Url.StartsWith(TEXT("Content/"), StringSearchCase::CaseSensitive))
            fileStream = FileReadStream::Open(Globals::ProjectFolder / info.Url);
        else
            fileStream = FileReadStream::Open(info.Url);
    }
    if (fileStream)
    {
        // File (AAsset* or Unix handle)
#if VIDEO_API_ANDROID_DEBUG
        LOG(Info, "[VideoBackendAndroid] Loading local file");
#endif
        auto* mediaSource = AMediaDataSource_new();
        AMediaDataSource_setUserdata(mediaSource, fileStream);
        AMediaDataSource_setReadAt(mediaSource, Android::AMediaDataSourceReadAt);
        AMediaDataSource_setGetSize(mediaSource, Android::AMediaDataSourceGetSize);
        AMediaDataSource_setClose(mediaSource, Android::AMediaDataSourceClose);
        status = AMediaExtractor_setDataSourceCustom(playerAndroid.Extractor, mediaSource);
    }
    else
    {
        // Url
#if VIDEO_API_ANDROID_DEBUG
        LOG(Info, "[VideoBackendAndroid] Loading url");
#endif
        const StringAsANSI<> url(info.Url.Get(), info.Url.Length());
        status = AMediaExtractor_setDataSource(playerAndroid.Extractor, url.Get());
    }
    if (status != AMEDIA_OK)
    {
        VIDEO_API_ANDROID_ERROR(AMediaExtractor_setDataSource, status);
        AMediaExtractor_delete(playerAndroid.Extractor);
        return true;
    }

    // Load tracks
    playerAndroid.VideoTrackIndex = playerAndroid.AudioTrackIndex = MAX_uint16;
    player.FrameRate = 24;
    size_t tracks = AMediaExtractor_getTrackCount(playerAndroid.Extractor);
    for (size_t trackIndex = 0; trackIndex < tracks; trackIndex++)
    {
        AMediaFormat* trackFormat = AMediaExtractor_getTrackFormat(playerAndroid.Extractor, trackIndex);
#if VIDEO_API_ANDROID_DEBUG
        const char* trackFormatName = AMediaFormat_toString(trackFormat);
        LOG(Info, "[VideoBackendAndroid] Track [{}]: {}", trackIndex, String(trackFormatName));
#endif
        const char* mime;
        if (AMediaFormat_getString(trackFormat, AMEDIAFORMAT_KEY_MIME, &mime))
        {
            if (playerAndroid.VideoCodec == nullptr && !strncmp(mime, "video/", 6))
            {
                // Video track
                playerAndroid.VideoCodec = AMediaCodec_createDecoderByType(mime);
                status = AMediaCodec_configure(playerAndroid.VideoCodec, trackFormat, nullptr, nullptr, 0);
                if (status != AMEDIA_OK)
                {
                    VIDEO_API_ANDROID_ERROR(AMediaCodec_configure, status);
                    AMediaCodec_delete(playerAndroid.VideoCodec);
                    playerAndroid.VideoCodec = nullptr;
                }
                else
                {
                    status = AMediaExtractor_selectTrack(playerAndroid.Extractor, trackIndex);
                    if (status != AMEDIA_OK)
                    {
                        VIDEO_API_ANDROID_ERROR(AMediaExtractor_selectTrack, status);
                        AMediaCodec_delete(playerAndroid.VideoCodec);
                        playerAndroid.VideoCodec = nullptr;
                    }
                    else
                    {
                        playerAndroid.VideoTrackIndex = trackIndex;
                        Android::UpdateFormat(player, playerAndroid, playerAndroid.VideoCodec, trackFormat);
                    }
                }
            }
            else if (playerAndroid.AudioCodec == nullptr && !strncmp(mime, "audio/", 6))
            {
                // Audio track
                playerAndroid.AudioCodec = AMediaCodec_createDecoderByType(mime);
                status = AMediaCodec_configure(playerAndroid.AudioCodec, trackFormat, nullptr, nullptr, 0);
                if (status != AMEDIA_OK)
                {
                    VIDEO_API_ANDROID_ERROR(AMediaCodec_configure, status);
                    AMediaCodec_delete(playerAndroid.AudioCodec);
                    playerAndroid.AudioCodec = nullptr;
                }
                else
                {
                    status = AMediaExtractor_selectTrack(playerAndroid.Extractor, trackIndex);
                    if (status != AMEDIA_OK)
                    {
                        VIDEO_API_ANDROID_ERROR(AMediaExtractor_selectTrack, status);
                        AMediaCodec_delete(playerAndroid.AudioCodec);
                        playerAndroid.AudioCodec = nullptr;
                    }
                    else
                    {
                        playerAndroid.AudioTrackIndex = trackIndex;
                        Android::UpdateFormat(player, playerAndroid, playerAndroid.AudioCodec, trackFormat);
                    }
                }
            }
        }
        AMediaFormat_delete(trackFormat);
    }

    // Setup player data
    player.Backend = this;
    playerAndroid.Loop = info.Loop;
    player.Created(info);
    Android::Players.Add(&player);

    return false;
}

void VideoBackendAndroid::Player_Destroy(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player.ReleaseResources();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    if (playerAndroid.VideoFormat)
        AMediaFormat_delete(playerAndroid.VideoFormat);
    if (playerAndroid.VideoCodec)
        AMediaCodec_delete(playerAndroid.VideoCodec);
    if (playerAndroid.AudioFormat)
        AMediaFormat_delete(playerAndroid.AudioFormat);
    if (playerAndroid.AudioCodec)
        AMediaCodec_delete(playerAndroid.AudioCodec);
    AMediaExtractor_delete(playerAndroid.Extractor);
    Android::Players.Remove(&player);
    player = VideoBackendPlayer();
}

void VideoBackendAndroid::Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    playerAndroid.Loop = info.Loop;
    player.Updated(info);
}

void VideoBackendAndroid::Player_Play(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    playerAndroid.Playing = 1;
    playerAndroid.InputEnded = playerAndroid.OutputEnded = 0;
    if (playerAndroid.VideoCodec)
        AMediaCodec_start(playerAndroid.VideoCodec);
    if (playerAndroid.AudioCodec)
        AMediaCodec_start(playerAndroid.AudioCodec);
    player.PlayAudio();
}

void VideoBackendAndroid::Player_Pause(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    playerAndroid.Playing = 0;
    if (playerAndroid.VideoCodec)
        AMediaCodec_stop(playerAndroid.VideoCodec);
    if (playerAndroid.AudioCodec)
        AMediaCodec_stop(playerAndroid.AudioCodec);
    player.PauseAudio();
}

void VideoBackendAndroid::Player_Stop(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    player.VideoFrameDuration = player.AudioBufferDuration = TimeSpan::Zero();
    playerAndroid.Playing = 0;
    playerAndroid.InputEnded = playerAndroid.OutputEnded = 0;
    media_status_t status = AMediaExtractor_seekTo(playerAndroid.Extractor, 0, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
    if (status != AMEDIA_OK)
    {
        VIDEO_API_ANDROID_ERROR(AMediaExtractor_seekTo, status);
    }
    if (playerAndroid.VideoCodec)
    {
        AMediaCodec_stop(playerAndroid.VideoCodec);
        AMediaCodec_flush(playerAndroid.VideoCodec);
    }
    if (playerAndroid.AudioCodec)
    {
        AMediaCodec_stop(playerAndroid.VideoCodec);
        AMediaCodec_flush(playerAndroid.VideoCodec);
    }
    player.StopAudio();
}

void VideoBackendAndroid::Player_Seek(VideoBackendPlayer& player, TimeSpan time)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    player.VideoFrameDuration = player.AudioBufferDuration = TimeSpan::Zero();
    media_status_t status = AMediaExtractor_seekTo(playerAndroid.Extractor, time.Ticks / 10, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
    if (status != AMEDIA_OK)
    {
        VIDEO_API_ANDROID_ERROR(AMediaExtractor_seekTo, status);
    }
    if (playerAndroid.VideoCodec)
        AMediaCodec_flush(playerAndroid.VideoCodec);
    if (playerAndroid.AudioCodec)
        AMediaCodec_flush(playerAndroid.AudioCodec);
    player.StopAudio();
}

TimeSpan VideoBackendAndroid::Player_GetTime(const VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAndroid = player.GetBackendState<VideoPlayerAndroid>();
    int64 time = AMediaExtractor_getSampleTime(playerAndroid.Extractor);
    if (time < 0)
        return TimeSpan::Zero();
    return TimeSpan(time * 10);
}

const Char* VideoBackendAndroid::Base_Name()
{
    return TEXT("Android NDK Media");
}

bool VideoBackendAndroid::Base_Init()
{
    return false;
}

void VideoBackendAndroid::Base_Update(TaskGraph* graph)
{
    // Schedule work to update all videos in async
    Function<void(int32)> job;
    job.Bind(Android::UpdatePlayer);
    graph->DispatchJob(job, Android::Players.Count());
}

void VideoBackendAndroid::Base_Dispose()
{
}

#endif
