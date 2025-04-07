// Copyright (c) Wojciech Figat. All rights reserved.

#include "MP3Decoder.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>

bool MP3Decoder::Convert(ReadStream* stream, AudioDataInfo& info, Array<byte>& result, uint32 offset)
{
    ASSERT(stream);

    mStream = stream;
    mStream->SetPosition(offset);

    int32 dataSize = mStream->GetLength() - offset;
    Array<byte> dataBytes;
    dataBytes.Resize(dataSize);
    byte* data = dataBytes.Get();
    mStream->ReadBytes(data, dataSize);

    info.NumSamples = 0;
    info.SampleRate = 0;
    info.NumChannels = 0;
    info.BitDepth = 16;

    mp3dec_frame_info_t mp3Info;
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

    MemoryWriteStream output(Math::RoundUpToPowerOf2(dataSize));

    do
    {
        const int32 samples = mp3dec_decode_frame(&mp3d, data, dataSize, pcm, &mp3Info);
        if (samples)
        {
            info.NumSamples += samples * mp3Info.channels;

            output.WriteBytes(pcm, samples * 2 * mp3Info.channels);

            if (!info.SampleRate)
                info.SampleRate = mp3Info.hz;
            if (!info.NumChannels)
                info.NumChannels = mp3Info.channels;
            if (info.SampleRate != mp3Info.hz || info.NumChannels != mp3Info.channels)
                break;
        }
        data += mp3Info.frame_bytes;
        dataSize -= mp3Info.frame_bytes;
    } while (mp3Info.frame_bytes);

    if (info.SampleRate == 0)
        return true;

    // Load the whole audio data
    const int32 bytesPerSample = info.BitDepth / 8;
    const int32 bufferSize = info.NumSamples * bytesPerSample;
    result.Set(output.GetHandle(), bufferSize);

    return false;
}

bool MP3Decoder::Open(ReadStream* stream, AudioDataInfo& info, uint32 offset)
{
    CRASH;
    // TODO: open MP3
    return true;
}

void MP3Decoder::Seek(uint32 offset)
{
    // TODO: seek MP3
    CRASH;
}

void MP3Decoder::Read(byte* samples, uint32 numSamples)
{
    // TODO: load MP3 format
    CRASH;
}

bool MP3Decoder::IsValid(ReadStream* stream, uint32 offset)
{
    // TODO: check MP3 format
    CRASH;
    return false;
}
