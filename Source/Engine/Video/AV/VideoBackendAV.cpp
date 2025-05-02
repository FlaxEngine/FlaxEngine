// Copyright (c) Wojciech Figat. All rights reserved.

#if VIDEO_API_AV

#include "VideoBackendAV.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/TaskGraph.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Globals.h"
#include <AVFoundation/AVFoundation.h>

#define VIDEO_API_AV_ERROR(api, err) LOG(Warning, "[VideoBackendAV] {} failed with error 0x{:x}", TEXT(#api), (uint64)err)

struct VideoPlayerAV
{
    AVPlayer* Player;
    AVPlayerItemVideoOutput* Output;
    int8 PendingPlay : 1;
    int8 PendingPause : 1;
    int8 PendingSeek : 1;
    TimeSpan SeekTime;
};

namespace AV
{
    Array<VideoBackendPlayer*> Players;

    TimeSpan ConvertTime(const CMTime& t)
    {
        return TimeSpan::FromSeconds(t.timescale != 0 ? (t.value / (double)t.timescale) : 0.0);
    }

    CMTime ConvertTime(const TimeSpan& t)
    {
        return CMTime{(CMTimeValue)(100000.0 * t.GetTotalSeconds()), (CMTimeScale)100000, kCMTimeFlags_Valid, {}};
    }

    void UpdatePlayer(int32 index)
    {
        PROFILE_CPU();
        auto& player = *Players[index];
        ZoneText(player.DebugUrl, player.DebugUrlLen);
        auto& playerAV = player.GetBackendState<VideoPlayerAV>();

        // Update format
        AVPlayerItem* playerItem = [playerAV.Player currentItem];
        if (!playerItem)
            return;
        if (player.Width == 0)
        {
            CGSize size = [playerItem presentationSize];
            player.Width = player.VideoFrameWidth = size.width;
            player.Height = player.VideoFrameHeight = size.height;
            NSArray* tracks = [playerItem tracks];
            for (NSUInteger i = 0; i < [tracks count]; i++)
            {
                AVPlayerItemTrack* track = (AVPlayerItemTrack*)[tracks objectAtIndex:i];
                AVAssetTrack* assetTrack = track.assetTrack;
                NSString* mediaType = assetTrack.mediaType;
                if ([mediaType isEqualToString:AVMediaTypeVideo] && playerAV.Output == nullptr)
		        {
                    player.FrameRate = assetTrack.nominalFrameRate;
                    if (player.FrameRate <= 0.0f)
                    {
                        CMTime frameDuration = assetTrack.minFrameDuration;
                        if ((frameDuration.flags & kCMTimeFlags_Valid) != 0)
                            player.FrameRate = (float)frameDuration.timescale / (float)frameDuration.value;
                        else
                            player.FrameRate = 25;
                    }
			        CGSize frameSize = assetTrack.naturalSize;
                    player.Width = player.VideoFrameWidth = frameSize.width;
                    player.Height = player.VideoFrameHeight = frameSize.height;

                    CMFormatDescriptionRef desc = (CMFormatDescriptionRef)[assetTrack.formatDescriptions objectAtIndex:0];
                    CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(desc);
                    int32 pixelFormat = kCVPixelFormatType_32BGRA; // TODO: use packed vieo format
                    player.Format = PixelFormat::B8G8R8A8_UNorm;

			        NSMutableDictionary* attributes = [NSMutableDictionary dictionary];
                    [attributes setObject:[NSNumber numberWithInt: pixelFormat] forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
			        [attributes setObject:[NSNumber numberWithInteger:1] forKey:(NSString*)kCVPixelBufferBytesPerRowAlignmentKey];

			        playerAV.Output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attributes];
				    [playerItem addOutput: playerAV.Output];
                }
                else if ([mediaType isEqualToString:AVMediaTypeAudio])
                {
                    CMFormatDescriptionRef desc = (CMFormatDescriptionRef)[assetTrack.formatDescriptions objectAtIndex:0];
                    const AudioStreamBasicDescription* audioDesc = CMAudioFormatDescriptionGetStreamBasicDescription(desc);
                    player.AudioInfo.SampleRate = audioDesc->mSampleRate;
                    player.AudioInfo.NumChannels = audioDesc->mChannelsPerFrame;
                    player.AudioInfo.BitDepth = audioDesc->mBitsPerChannel > 0 ? audioDesc->mBitsPerChannel : 16;
                }
            }
        }

        // Wait for the video to be ready
        //AVPlayerStatus status = [playerAV.Player status];
        //AVPlayerTimeControlStatus timeControlStatus = [playerAV.Player timeControlStatus];
        if (playerAV.Output == nullptr)
            return;

        // Control playback
        if (playerAV.PendingPlay)
        {
            playerAV.PendingPlay = 0;
            [playerAV.Player play];
        }
        else if (playerAV.PendingPause)
        {
            playerAV.PendingPause = 0;
            [playerAV.Player pause];
        }
        if (playerAV.PendingSeek)
        {
            playerAV.PendingSeek = 0;
            [playerAV.Player seekToTime:AV::ConvertTime(playerAV.SeekTime)];
            //[playerAV.Player seekToTime:time toleranceBefore:time toleranceAfter:time];
        }

        // Check if there is a new video frame to process
        CMTime currentTime = [playerAV.Player currentTime];
        if (playerAV.Output && [playerAV.Output hasNewPixelBufferForItemTime: currentTime])
        {
            CVPixelBufferRef buffer = [playerAV.Output copyPixelBufferForItemTime:currentTime itemTimeForDisplay:nullptr];
            if (buffer)
            {
                const int32 bufferWidth = CVPixelBufferGetWidth(buffer);
                const int32 bufferHeight = CVPixelBufferGetHeight(buffer);
                const int32 bufferStride = CVPixelBufferGetBytesPerRow(buffer);
                const int32 bufferSize = bufferStride * bufferHeight;

                // TODO: use Metal Texture Cache for faster GPU-based video processing

                if (CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess)
                {
                    uint8* bufferData = (uint8*)CVPixelBufferGetBaseAddress(buffer);
                    player.UpdateVideoFrame(Span<byte>(bufferData, bufferSize), ConvertTime(currentTime), TimeSpan::FromSeconds(1.0f / player.FrameRate));
                    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
                }

	            CVPixelBufferRelease(buffer);
            }
        }

        player.Tick();
    }
}

bool VideoBackendAV::Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player = VideoBackendPlayer();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();

    // Load media
    NSURL* url;
    if (info.Url.StartsWith(TEXT("http"), StringSearchCase::IgnoreCase))
    {
        url = [NSURL URLWithString:(NSString*)AppleUtils::ToString(info.Url)];

    }
    else
    {
#if PLATFORM_MAC
        if (info.Url.StartsWith(TEXT("Content/"), StringSearchCase::CaseSensitive))
            url = [NSURL fileURLWithPath:(NSString*)AppleUtils::ToString(Globals::ProjectFolder / info.Url) isDirectory:NO];
        else
            url = [NSURL fileURLWithPath:(NSString*)AppleUtils::ToString(info.Url) isDirectory:NO];
#else
        url = [NSURL fileURLWithPath:(NSString*)AppleUtils::ToString(StringUtils::GetFileName(info.Url)) isDirectory:NO];
#endif
    }
    playerAV.Player = [AVPlayer playerWithURL:url];
    if (playerAV.Player == nullptr)
    {
        return true;
    }
    [playerAV.Player retain];

    // Configure player
    //[playerAV.Player addObserver:playerStatusObserver.get() forKeyPath:"status" options:NSKeyValueObservingOptionNew context:&player];
	playerAV.Player.actionAtItemEnd = info.Loop ? AVPlayerActionAtItemEndNone : AVPlayerActionAtItemEndPause;
    [playerAV.Player setVolume: info.Volume];

    // Setup player data
    player.Backend = this;
    player.Created(info);
    AV::Players.Add(&player);

    return false;
}

void VideoBackendAV::Player_Destroy(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player.ReleaseResources();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    if (playerAV.PendingPause)
        [playerAV.Player pause];
    if (playerAV.Output)
        [playerAV.Output release];
    [playerAV.Player release];
    AV::Players.Remove(&player);
    player = VideoBackendPlayer();
}

void VideoBackendAV::Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
	playerAV.Player.actionAtItemEnd = info.Loop ? AVPlayerActionAtItemEndNone : AVPlayerActionAtItemEndPause;
    // TODO: spatial audio
    // TODO: audio pan
    [playerAV.Player setVolume: info.Volume];
    player.Updated(info);
}

void VideoBackendAV::Player_Play(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    playerAV.PendingPlay = true;
    playerAV.PendingPause = false;
    player.PlayAudio();
}

void VideoBackendAV::Player_Pause(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    playerAV.PendingPlay = false;
    playerAV.PendingPause = true;
    player.PauseAudio();
}

void VideoBackendAV::Player_Stop(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    playerAV.PendingPlay = false;
    playerAV.PendingPause = true;
    playerAV.PendingSeek = true;
    playerAV.SeekTime = TimeSpan::Zero();
    player.StopAudio();
}

void VideoBackendAV::Player_Seek(VideoBackendPlayer& player, TimeSpan time)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    playerAV.PendingSeek = true;
    playerAV.SeekTime = time;
}

TimeSpan VideoBackendAV::Player_GetTime(const VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerAV = player.GetBackendState<VideoPlayerAV>();
    if (playerAV.PendingSeek)
        return playerAV.SeekTime;
    return AV::ConvertTime([playerAV.Player currentTime]);
}

const Char* VideoBackendAV::Base_Name()
{
    return TEXT("AVFoundation");
}

bool VideoBackendAV::Base_Init()
{
    return false;
}

void VideoBackendAV::Base_Update(TaskGraph* graph)
{
    // Schedule work to update all videos in async
    Function<void(int32)> job;
    job.Bind(AV::UpdatePlayer);
    graph->DispatchJob(job, AV::Players.Count());
}

void VideoBackendAV::Base_Dispose()
{
}

#endif
