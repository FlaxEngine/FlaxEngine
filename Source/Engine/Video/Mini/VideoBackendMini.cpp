// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if VIDEO_API_MINI

#include <vector>
#include <queue>
#include "VideoBackendMini.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/TaskGraph.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Engine/Time.h"
#include "Engine/Audio/Types.h"
#include "Engine/Audio/AudioBackend.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#define MINIMP4_IMPLEMENTATION
#include <ThirdParty/minimp4/minimp4.h>
#include <ThirdParty/openh264/codec_api.h>
#include <ThirdParty/fdkaac/aacenc_lib.h>
#include <ThirdParty/fdkaac/aacdecoder_lib.h>

#define VIDEO_API_MINI_ERROR(api, err) LOG(Warning, "[VideoBackendMini] {} failed with error 0x{:x}", TEXT(#api), (uint64)err)
#define SHORT_SYNC 0

struct SampleInfo
{
    size_t index;
    uint32 offset;
    unsigned size;
    TimeSpan timestamp;
    TimeSpan duration;
};

struct VideoPlayerMini
{
    FileReadStream* Stream;
    ISVCDecoder* VideoDecoder = nullptr;
    HANDLE_AACDECODER AudioDecoder;
    std::vector<SampleInfo> VideoSamplesInfo;
    std::vector<SampleInfo> AudioSamplesInfo;
    std::vector<uint8_t> InitialBuffer;
    size_t currentVideoIndex = 0;
    size_t currentAudioIndex = 0;
    uint8 DecodeRestart : 1;
    uint8 Loop : 1;
    uint8 Playing : 1;
    uint8 FirstFrame : 1;
    uint8 Seek : 1;
    TimeSpan Time;
};

namespace Mini
{
    Array<VideoBackendPlayer*> Players;
    
    int ReadCallback(int64_t offset, void* buffer, size_t size, void* token)
    {
        auto* stream = (FileReadStream*)token;
        stream->SetPosition((uint32)offset);
        stream->ReadBytes(buffer, size);
        return stream->HasError();
    }

    bool ResetVideoDecoder(VideoPlayerMini& playerMini)
    {
        // If created, destroy
        if (playerMini.VideoDecoder != nullptr)
        {
            playerMini.VideoDecoder->Uninitialize();
            WelsDestroyDecoder(playerMini.VideoDecoder);
        }
        
        // Initialize Video decoder 
        if (WelsCreateDecoder(&playerMini.VideoDecoder) != 0)
        {
            LOG(Warning, "Failed to create openh264 decoder");
            return true;
        }

        SDecodingParam decParam = {0};
        decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
        if (playerMini.VideoDecoder->Initialize(&decParam) != 0)
        {
            LOG(Warning, "Failed to initialize openh264 decoder");
            return true;
        }
        playerMini.DecodeRestart = 1;
        return false;
    }

    void ConfigureVideo(VideoBackendPlayer& player, VideoPlayerMini& playerMini, MP4D_demux_t& mp4, unsigned trackIndex)
    {
        MP4D_track_t& track = mp4.track[trackIndex];
        player.Width = player.VideoFrameWidth = track.SampleDescription.video.width;
        player.Height = player.VideoFrameHeight = track.SampleDescription.video.height;
        player.Format = PixelFormat::YUY2;
        
        player.Duration = TimeSpan::FromSeconds((static_cast<double>(mp4.duration_hi) * 0x100000000L + mp4.duration_lo) / mp4.timescale);
        double durationSeconds = static_cast<double>(player.Duration.Ticks) / TimeSpan::TicksPerSecond;
        player.FrameRate = static_cast<float>(track.sample_count / durationSeconds);
        LOG(Info, "Video track: {0}x{1}, framerate: {2}, duration: {3}", player.Width, player.Height, player.FrameRate, player.Duration);

        ResetVideoDecoder(playerMini);
    }

    bool ConfigureAudio(VideoBackendPlayer& player, VideoPlayerMini& playerMini, MP4D_demux_t& mp4, unsigned trackIndex)
    {
        MP4D_track_t& track = mp4.track[trackIndex];
        //player.AudioInfo.SampleRate = track.SampleDescription.audio.samplerate_hz;
        player.AudioInfo.NumChannels = track.SampleDescription.audio.channelcount;
        player.AudioInfo.BitDepth = 16;

        // Initalize Audio Decoder
        playerMini.AudioDecoder = aacDecoder_Open(TT_MP4_RAW, 1);
        if (!playerMini.AudioDecoder)
        {
            LOG(Warning, "Error: could not initialize AAC decoder");
            return true;
        }
        UCHAR *dsi =(UCHAR *)track.dsi;
        UINT dsi_size = track.dsi_bytes;
        if (AAC_DEC_OK != aacDecoder_ConfigRaw(playerMini.AudioDecoder, &dsi, &dsi_size))
        {
             LOG(Warning, "Error: AAC config failure");
            return true;
        }

        // Get samplerate
        CStreamInfo* info = aacDecoder_GetStreamInfo(playerMini.AudioDecoder);
        if (info)
        {
            uint32 sampleRate = info->sampleRate == 0 ? info->aacSampleRate : info->sampleRate;
            player.AudioInfo.SampleRate = sampleRate;
        }
        else
        {
            LOG(Warning, "Error: Could not get audio track info");
            return true;
        }            
        LOG(Info, "Audio track: samplerate: {0}hz, bitdepth: {1}, channels: {2}", 
            player.AudioInfo.SampleRate, player.AudioInfo.BitDepth, player.AudioInfo.NumChannels);
        
        return false;
    }

    void DemuxTrack(MP4D_demux_t& mp4, std::vector<SampleInfo>& outSamples, int trackIndex)
    {
        auto& track = mp4.track[trackIndex];

        for (size_t i = 0; i < track.sample_count; i++)
        {
            unsigned frame_bytes, timestamp, frame_duration;
            auto offset = MP4D_frame_offset(&mp4, trackIndex, i, &frame_bytes, &timestamp, &frame_duration);
            SampleInfo sample;
            sample.index = i;
            sample.offset = offset;
            sample.size = frame_bytes;
            sample.timestamp = TimeSpan::FromSeconds((double) timestamp / track.timescale);
            sample.duration = TimeSpan::FromSeconds((double) frame_duration / track.timescale);

            outSamples.push_back(sample);
        }
    }

    void appendToBuffer(std::vector<uint8_t>& buffer, const void* data, ssize_t size)
    {
        if (!data || size == 0)
            return;
        
        size_t oldSize = buffer.size();
        buffer.resize(oldSize + size);
        memcpy(buffer.data() + oldSize, data, size);
    }

    void createInitialBuffer(MP4D_demux_t& mp4, unsigned int track, std::vector<uint8_t>& buffer)
    {
        int spspps_bytes;
        const void* spspps;
        
        char sync[4] = { 0, 0, 0, 1 };
        
        for (int i = 0; const void* spspps = MP4D_read_sps(&mp4, track, i, &spspps_bytes); ++i)
        {
            appendToBuffer(buffer, sync + SHORT_SYNC, 4 - SHORT_SYNC);
            appendToBuffer(buffer, spspps, spspps_bytes);
        }

        for (int i = 0; const void* spspps = MP4D_read_pps(&mp4, track, i, &spspps_bytes); ++i)
        {
            appendToBuffer(buffer, sync + SHORT_SYNC, 4 - SHORT_SYNC);
            appendToBuffer(buffer, spspps, spspps_bytes);
        }
    }

    std::vector<uint8_t> convertToYUY2(uint8_t* pData[3], SBufferInfo& bufferInfo)
    {
        int width  = bufferInfo.UsrData.sSystemBuffer.iWidth;
        int height = bufferInfo.UsrData.sSystemBuffer.iHeight;

        int strideY = bufferInfo.UsrData.sSystemBuffer.iStride[0];
        int strideUV = bufferInfo.UsrData.sSystemBuffer.iStride[1];
        //int strideV = bufferInfo.UsrData.sSystemBuffer.iStride[2];

        int rowPitch   = ((width + 1) >> 1) * 4;
        int slicePitch = rowPitch * height;

        std::vector<uint8_t> frameData(slicePitch);
        auto dst = frameData.data();

        uint8_t* srcY = pData[0];
        uint8_t* srcU = pData[1];
        uint8_t* srcV = pData[2];

        for (int y = 0; y < height; y++)
        {
            uint8_t* dstRow = dst + y * rowPitch;
            uint8_t* rowY = srcY + y * strideY;
            uint8_t* rowU = srcU + (y / 2) * strideUV;
            uint8_t* rowV = srcV + (y / 2) * strideUV;

            for (int x = 0; x < width; x += 2)
            {
                uint8_t Y0 = rowY[x];
                uint8_t Y1 = rowY[x + 1];
                uint8_t U  = rowU[x / 2];
                uint8_t V  = rowV[x / 2];

                int dstIndex = (x / 2) * 4;
                dstRow[dstIndex + 0] = Y0;
                dstRow[dstIndex + 1] = U;
                dstRow[dstIndex + 2] = Y1;
                dstRow[dstIndex + 3] = V;
            }
        }
        return frameData;
    }

    bool DecodeVideoSample(VideoBackendPlayer& player, VideoPlayerMini& playerMini, size_t index, std::deque<std::vector<uint8_t>>& videoFrame)
    {
        PROFILE_CPU_NAMED("DecodeVideoSample");

        if (index > playerMini.VideoSamplesInfo.size())
        {
            LOG(Warning, "Error: sample {0} requested is greater than total samples {1}", index, playerMini.VideoSamplesInfo.size());
            return true;
        }
            
        auto& sample = playerMini.VideoSamplesInfo[index];
        unsigned frame_bytes = sample.size;
        
        // Reserve memory
        std::vector<uint8_t> memory(frame_bytes);

        // Read sample
        playerMini.Stream->SetPosition(sample.offset);
        playerMini.Stream->ReadBytes(memory.data(), frame_bytes);

        std::vector<uint8_t> frameBuffer;
        if (playerMini.DecodeRestart)
        {
            frameBuffer = playerMini.InitialBuffer;
            playerMini.DecodeRestart = 0;
            //LOG(Info, "Sps/pps data sent with sample {0}", index);
        }
        else
            frameBuffer.clear();

        auto mem = memory.data();
        while (frame_bytes)
        {
            uint32_t size = ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) 
                | ((uint32_t)mem[2] << 8) | mem[3];
            size += 4;
            mem[0] = mem[1] = mem[2] = 0; 
            mem[3] = 1;
            
            appendToBuffer(frameBuffer, mem + SHORT_SYNC, size - SHORT_SYNC);
            if (frame_bytes < size)
            {
                LOG(Warning, "Error demuxing mp4 video sample");
                return true;
            }

            frame_bytes -= size;
            mem += size;    
        }

        // Decode Frame
        uint8_t* pData[3] = {nullptr, nullptr, nullptr};
        SBufferInfo bufferInfo;
        memset(&bufferInfo, 0, sizeof(bufferInfo));
        
        int iRet = playerMini.VideoDecoder->DecodeFrameNoDelay(frameBuffer.data(), frameBuffer.size(), pData, &bufferInfo);
        if (iRet != 0)
        {
            LOG(Warning, "Error decoding frame {0}", index);
            return true;
        }
        
        if (bufferInfo.iBufferStatus == 1)
        {
            // Convert to the format and push it to queue
            videoFrame.push_back(convertToYUY2(pData, bufferInfo));
            return false;
        }
        else
            return true;
    }

    bool DecodeAudioSample(VideoBackendPlayer& player, VideoPlayerMini& playerMini, size_t index, std::deque<std::vector<byte>>& audioBuffer)
    {       
        PROFILE_CPU_NAMED("DecodeAudioSample");
        
        auto& sample = playerMini.AudioSamplesInfo[index];

        std::vector<UCHAR> frame(sample.size);
        playerMini.Stream->SetPosition(sample.offset);
        playerMini.Stream->ReadBytes(frame.data(), sample.size);
        UCHAR* dataPtr = frame.data();

        unsigned frame_size = sample.size;
        unsigned valid = frame_size;
        if (AAC_DEC_OK != aacDecoder_Fill(playerMini.AudioDecoder, &dataPtr, &frame_size, &valid))
        {
            LOG(Warning, "Error: aac decode fail");
            return true;
        }

        std::vector<INT_PCM> pcm;
        pcm.resize(2048 * 8);
        int err = aacDecoder_DecodeFrame(playerMini.AudioDecoder, pcm.data(), pcm.size(), 0);
        if (err != AAC_DEC_OK)
        {
            LOG(Warning, "Error decoding aac frame");
            return true;
        }

        CStreamInfo* info = aacDecoder_GetStreamInfo(playerMini.AudioDecoder);
        if (!info)
        {
            LOG(Warning, "aac StreamInfo error");
            return true;
        }

        // Copy decoded audio
        std::vector<byte> buffer;
        buffer.resize(sizeof(INT_PCM) * info->frameSize * info->numChannels);
        memcpy(buffer.data(), pcm.data(), buffer.size());
        
        // Send decoded sample to queue
        audioBuffer.push_back(buffer);

        return false;
    }

    size_t SeekSampleIndex(VideoBackendPlayer& player, VideoPlayerMini& playerMini, std::vector<SampleInfo>& sampleInfo)
    {        
        double targetTime = playerMini.Time.GetTotalSeconds();
        double bestTime = player.Duration.GetTotalSeconds();

        int low = 0;
        int high = (int)sampleInfo.size() - 1;
        int bestIndex = 0;

        while (low <= high)
        {
            int mid = (low +high) / 2;
            double sampleTime = sampleInfo[mid].timestamp.GetTotalSeconds();
            double diff = std::abs(sampleTime - targetTime);

            if (diff < bestTime)
            {
                bestTime = diff;
                bestIndex = mid;
            }

            if (sampleTime < targetTime)
                low = mid + 1;
            else
                high = mid - 1;
        }
        return  bestIndex;
    }

    void UpdatePlayer(int32 index)
    {
        PROFILE_CPU();
        auto& player = *Players[index];
        ZoneText(player.DebugUrl, player.DebugUrlLen);
        auto& playerMini = player.GetBackendState<VideoPlayerMini>();

        // Skip paused player
        if (!playerMini.Playing && !playerMini.Seek)
            return;

        bool useTimeScale = true;
#if USE_EDITOR
        if (!Editor::IsPlayMode)
            useTimeScale = false;
#endif
        TimeSpan dt = useTimeScale ? Time::Update.DeltaTime : Time::Update.UnscaledDeltaTime;

        // Update playback time
        if (playerMini.FirstFrame)
        {
            playerMini.FirstFrame = 0;
            playerMini.Seek = 1;
        }
        else if (playerMini.Playing)
        {
            playerMini.Time += dt;
        }
        if (playerMini.Time > player.Duration)
        {
            if (playerMini.Loop)
            {
                // Loop
                LOG(Info, "Playback loop");
                //playerMini.Time.Ticks %= player.Duration.Ticks;
                playerMini.Time = TimeSpan::Zero();                
                playerMini.Seek = 1;    
            }
            else
            {
                // End
                LOG(Info, "Playback end");
                playerMini.Time = player.Duration;
                playerMini.Playing = 0;          
            }
        }

        // Update current position
        if (playerMini.Seek)
        {            
            // Reset cached frames timings
            player.VideoFrameDuration = player.AudioBufferDuration = TimeSpan::Zero();

            // Reset VideoDecoder
            ResetVideoDecoder(playerMini);

            playerMini.Seek = 0;
            
            playerMini.currentVideoIndex = SeekSampleIndex(player, playerMini, playerMini.VideoSamplesInfo);
            playerMini.currentAudioIndex = SeekSampleIndex(player, playerMini, playerMini.AudioSamplesInfo);  
        }

        // Update streams
        if (playerMini.Playing)
        {   
            std::deque<std::vector<uint8_t>> videoFrame;
            auto& index = playerMini.currentVideoIndex;
            auto& sample = playerMini.VideoSamplesInfo[index];

            if (sample.timestamp <= playerMini.Time)
            {
                if(!(DecodeVideoSample(player, playerMini, index, videoFrame)))
                {
                    auto& frame = videoFrame.front();
                    player.UpdateVideoFrame(Span<byte>(frame.data(), frame.size()),
                        sample.timestamp, sample.duration);                                     
                    videoFrame.pop_front();
                }
                // Advance next sample
                index = (index + 1) % playerMini.VideoSamplesInfo.size();
            }
        }
        // Audio
        if (player.AudioInfo.BitDepth != 0)
        {   
            const int BUFFER_LIMIT = 30;
            std::deque<std::vector<byte>> audioBuffer;
            size_t& index = playerMini.currentAudioIndex;

            int processed = 0;
            AudioBackend::Source::GetProcessedBuffersCount(player.AudioSource, processed);
            if (processed > 0)
                AudioBackend::Source::DequeueProcessedBuffers(player.AudioSource);

            int queued = 0;
            AudioBackend::Source::GetQueuedBuffersCount(player.AudioSource, queued);

            // If no queued buffers, force audio to play
            if (queued == 0)
            {
                player.IsAudioPlayPending = 1;
            }

            auto currentTime = playerMini.Time;
            int buffersAdded = 0;
            while (queued + buffersAdded < BUFFER_LIMIT)
            {
                auto& sample = playerMini.AudioSamplesInfo[index];

                if (sample.timestamp <= currentTime)
                {
                    if (!DecodeAudioSample(player, playerMini, index, audioBuffer))
                    {
                        auto& buffer = audioBuffer.front();
                        player.UpdateAudioBuffer(Span<byte>(buffer.data(), buffer.size()),
                            sample.timestamp, sample.duration);
                        audioBuffer.pop_front();
                        buffersAdded++;
                    }
                }
                
                // Advance next sample
                index = (index + 1) % playerMini.AudioSamplesInfo.size(); 
            }           
        }
            
        player.Tick();
    }
}

bool VideoBackendMini::Player_Create(const VideoBackendPlayerInfo& info, VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player = VideoBackendPlayer();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();

    // Open file
    playerMini.Stream = FileReadStream::Open(info.Url);
    if (!playerMini.Stream)
    {
        LOG(Warning, "[VideoBackendMini] Failed to open file '{}'", info.Url);
        return true;
    }

    // Load media data
    MP4D_demux_t mp4;
    {
        PROFILE_CPU_NAMED("MP4D_open");
        if (MP4D_open(&mp4, Mini::ReadCallback, playerMini.Stream, playerMini.Stream->GetLength()) != 1)
        {
            LOG(Warning, "[VideoBackendMini] Failed to parse mp4 file '{}'", info.Url);
            return true;
        }
    }

    for (unsigned trackIndex = 0; trackIndex < mp4.track_count; trackIndex++)
    {
        MP4D_track_t& track = mp4.track[trackIndex];
        if (track.handler_type == MP4D_HANDLER_TYPE_VIDE)
        {
            if (track.object_type_indication == MP4_OBJECT_TYPE_AVC)
            {
                Mini::ConfigureVideo(player, playerMini, mp4, trackIndex);

                // Create Video SamplesInfo
                Mini::DemuxTrack(mp4, playerMini.VideoSamplesInfo, trackIndex);

                // Create initial buffer
                Mini::createInitialBuffer(mp4, trackIndex, playerMini.InitialBuffer);
            }
            else if (track.object_type_indication == MP4_OBJECT_TYPE_HEVC)
            {
                LOG(Warning, "[VideoBackendMini] H.265 (HEVC) video format is not supported");
                continue;
            }
            else
            {
                LOG(Warning, "[VideoBackendMini] Unsupported video format");
                continue;
            }
        }
        else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN)
        {
            if (Mini::ConfigureAudio(player, playerMini, mp4, trackIndex))
                LOG(Warning, "Failed to configure mini backend audio settings");

            // Create Audio SamplesInfo
            Mini::DemuxTrack(mp4, playerMini.AudioSamplesInfo, trackIndex);
        }
    }

    // Setup player data
    player.Backend = this;
    playerMini.Loop = info.Loop;
    playerMini.FirstFrame = 1;
    player.Created(info);
    Mini::Players.Add(&player);

    MP4D_close(&mp4);

    return false;
}

void VideoBackendMini::Player_Destroy(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    player.ReleaseResources();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    Delete(playerMini.Stream);
    
    playerMini.VideoDecoder->Uninitialize();
    WelsDestroyDecoder(playerMini.VideoDecoder);
    aacDecoder_Close(playerMini.AudioDecoder);
    
    Mini::Players.Remove(&player);
    player = VideoBackendPlayer();
    LOG(Info, "player destroyed");
}

void VideoBackendMini::Player_UpdateInfo(VideoBackendPlayer& player, const VideoBackendPlayerInfo& info)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    playerMini.Loop = info.Loop;
    player.Updated(info);
}

void VideoBackendMini::Player_Play(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    playerMini.Playing = 1;
    player.PlayAudio();
}

void VideoBackendMini::Player_Pause(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    playerMini.Playing = 0;
    player.PauseAudio();
}

void VideoBackendMini::Player_Stop(VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    playerMini.Time = TimeSpan::Zero();
    playerMini.Playing = 0;
    playerMini.FirstFrame = 1;
    playerMini.Seek = 1;    
    player.StopAudio();
}

void VideoBackendMini::Player_Seek(VideoBackendPlayer& player, TimeSpan time)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    if (playerMini.Time != time)
    {
        playerMini.Time = time;
        playerMini.Seek = 1;
        player.StopAudio();
    }
}

TimeSpan VideoBackendMini::Player_GetTime(const VideoBackendPlayer& player)
{
    PROFILE_CPU();
    auto& playerMini = player.GetBackendState<VideoPlayerMini>();
    return playerMini.Time;
}

const Char* VideoBackendMini::Base_Name()
{
    return TEXT("minimp4");
}

bool VideoBackendMini::Base_Init()
{
    return false;
}

void VideoBackendMini::Base_Update(TaskGraph* graph)
{
    // Schedule work to update all videos models in async
    Function<void(int32)> job;
    job.Bind(Mini::UpdatePlayer);
    graph->DispatchJob(job, Mini::Players.Count());
}

void VideoBackendMini::Base_Dispose()
{
}

#endif
