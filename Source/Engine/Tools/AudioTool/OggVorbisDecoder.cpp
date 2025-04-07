// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_OGG_VORBIS

#include "OggVorbisDecoder.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include <vorbis/codec.h>

size_t oggRead(void* ptr, size_t size, size_t nmemb, void* data)
{
    OggVorbisDecoder* decoderData = static_cast<OggVorbisDecoder*>(data);
    const auto len = Math::Min<uint32>(static_cast<uint32>(size * nmemb), decoderData->Stream->GetLength() - decoderData->Stream->GetPosition());
    decoderData->Stream->ReadBytes(ptr, len);
    return static_cast<std::size_t>(len);
}

int oggSeek(void* data, ogg_int64_t offset, int whence)
{
    OggVorbisDecoder* decoderData = static_cast<OggVorbisDecoder*>(data);
    switch (whence)
    {
    case SEEK_SET:
        offset += decoderData->Offset;
        break;
    case SEEK_CUR:
        offset += decoderData->Stream->GetPosition();
        break;
    case SEEK_END:
        offset = Math::Max<ogg_int64_t>(0, decoderData->Stream->GetLength() - 1);
        break;
    }

    decoderData->Stream->SetPosition(static_cast<uint32>(offset));
    return static_cast<int>(decoderData->Stream->GetPosition() - decoderData->Offset);
}

long oggTell(void* data)
{
    OggVorbisDecoder* decoderData = static_cast<OggVorbisDecoder*>(data);
    return static_cast<long>(decoderData->Stream->GetPosition() - decoderData->Offset);
}

bool OggVorbisDecoder::Open(ReadStream* stream, AudioDataInfo& info, uint32 offset)
{
    if (stream == nullptr)
        return false;

    stream->SetPosition(offset);
    Stream = stream;
    Offset = offset;

    const ov_callbacks callbacks = { &oggRead, &oggSeek, nullptr, &oggTell };
    const int status = ov_open_callbacks(this, &OggVorbisFile, nullptr, 0, callbacks);
    if (status < 0)
    {
        LOG(Warning, "Failed to open Ogg Vorbis file.");
        return false;
    }

    vorbis_info* vorbisInfo = ov_info(&OggVorbisFile, -1);
    info.NumChannels = vorbisInfo->channels;
    info.SampleRate = vorbisInfo->rate;
    info.NumSamples = static_cast<uint32>(ov_pcm_total(&OggVorbisFile, -1) * vorbisInfo->channels);
    info.BitDepth = 16;

    ChannelCount = info.NumChannels;
    return true;
}

void OggVorbisDecoder::Seek(uint32 offset)
{
    ov_pcm_seek(&OggVorbisFile, offset / ChannelCount);
}

void OggVorbisDecoder::Read(byte* samples, uint32 numSamples)
{
    uint32 numReadSamples = 0;
    while (numReadSamples < numSamples)
    {
        const int32 bytesToRead = static_cast<int32>(numSamples - numReadSamples) * sizeof(int16);
        const uint32 bytesRead = ov_read(&OggVorbisFile, (char*)samples, bytesToRead, 0, 2, 1, nullptr);
        if (bytesRead > 0)
        {
            const uint32 samplesRead = bytesRead / sizeof(int16);
            numReadSamples += samplesRead;
            samples += samplesRead * sizeof(int16);
        }
        else
        {
            break;
        }
    }
}

bool OggVorbisDecoder::IsValid(ReadStream* stream, uint32 offset)
{
    stream->SetPosition(offset);
    Stream = stream;
    Offset = offset;

    OggVorbis_File file;
    const ov_callbacks callbacks = { &oggRead, &oggSeek, nullptr, &oggTell };
    if (ov_test_callbacks(this, &file, nullptr, 0, callbacks) == 0)
    {
        ov_clear(&file);
        return true;
    }

    return false;
}

#endif
