// Copyright (c) Wojciech Figat. All rights reserved.

#include "WaveDecoder.h"
#include "Engine/Core/Log.h"
#include "AudioTool.h"

#define WAVE_FORMAT_PCM 0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_ALAW 0x0006
#define WAVE_FORMAT_MULAW 0x0007
#define WAVE_FORMAT_EXTENDED 0xFFFE
#define MAIN_CHUNK_SIZE 12

bool WaveDecoder::ParseHeader(AudioDataInfo& info)
{
    bool foundData = false;
    while (!foundData)
    {
        // Get sub-chunk ID and size
        uint8 subChunkId[4];
        mStream->ReadBytes(subChunkId, sizeof(subChunkId));

        uint32 subChunkSize = 0;
        mStream->ReadUint32(&subChunkSize);

        uint32 totalRead = 0;

        // FMT chunk
        if (subChunkId[0] == 'f' && subChunkId[1] == 'm' && subChunkId[2] == 't' && subChunkId[3] == ' ')
        {
            uint16 format;
            mStream->ReadUint16(&format);
            totalRead += 2;

            if (format != WAVE_FORMAT_PCM && format != WAVE_FORMAT_IEEE_FLOAT && format != WAVE_FORMAT_EXTENDED)
            {
                LOG(Warning, "Wave file doesn't contain raw PCM data. Not supported.");
                return false;
            }

            uint16 numChannels = 0;
            mStream->ReadUint16(&numChannels);
            totalRead += 2;

            uint32 sampleRate = 0;
            mStream->ReadUint32(&sampleRate);
            totalRead += 4;

            uint32 byteRate = 0;
            mStream->ReadUint32(&byteRate);
            totalRead += 4;

            uint16 blockAlign = 0;
            mStream->ReadUint16(&blockAlign);
            totalRead += 2;

            uint16 bitDepth = 0;
            mStream->ReadUint16(&bitDepth);
            totalRead += 2;

            if (bitDepth != 8 && bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
            {
                LOG(Warning, "Unsupported number of bits per sample: {0}", bitDepth);
                return false;
            }

            info.NumChannels = numChannels;
            info.SampleRate = sampleRate;
            info.BitDepth = bitDepth;

            // Read extension data, and get the actual format
            if (format == WAVE_FORMAT_EXTENDED)
            {
                uint16 extensionSize = 0;
                mStream->ReadUint16(&extensionSize);
                totalRead += 2;

                if (extensionSize != 22)
                {
                    LOG(Warning, "Wave file doesn't contain raw PCM data. Not supported.");
                    return false;
                }

                uint16 validBitDepth = 0;
                mStream->ReadUint16(&validBitDepth);
                totalRead += 2;

                uint32 channelMask = 0;
                mStream->ReadUint32(&channelMask);
                totalRead += 4;

                uint8 subFormat[16];
                mStream->ReadBytes(subFormat, sizeof(subFormat));
                totalRead += 16;

                Platform::MemoryCopy(&format, subFormat, sizeof(format));
                if (format != WAVE_FORMAT_PCM)
                {
                    LOG(Warning, "Wave file doesn't contain raw PCM data. Not supported.");
                    return false;
                }
            }

            // Support wav with "extra format bytes", just ignore not needed
            while (totalRead < subChunkSize)
            {
                uint8 b;
                mStream->ReadBytes(&b, sizeof(b));
                totalRead++;
            }

            mBytesPerSample = bitDepth / 8;
            mFormat = format;
        }
        // DATA chunk
        else if (subChunkId[0] == 'd' && subChunkId[1] == 'a' && subChunkId[2] == 't' && subChunkId[3] == 'a')
        {
            info.NumSamples = subChunkSize / mBytesPerSample;
            mDataOffset = (uint32)mStream->GetPosition();

            foundData = true;
        }
        // Unsupported chunk type
        else
        {
            if (mStream->GetPosition() + subChunkSize >= mStream->GetLength())
                return false;
            mStream->SetPosition(mStream->GetPosition() + subChunkSize);
        }
    }

    return true;
}

bool WaveDecoder::Open(ReadStream* stream, AudioDataInfo& info, uint32 offset)
{
    ASSERT(stream);

    mStream = stream;
    mStream->SetPosition(offset + MAIN_CHUNK_SIZE);

    if (!ParseHeader(info))
    {
        LOG(Warning, "Provided file is not a valid WAVE file.");
        return false;
    }

    return true;
}

void WaveDecoder::Seek(uint32 offset)
{
    mStream->SetPosition(mDataOffset + offset * mBytesPerSample);
}

void WaveDecoder::Read(byte* samples, uint32 numSamples)
{
    const uint32 numRead = numSamples * mBytesPerSample;
    mStream->ReadBytes(samples, numRead);

    // 8-bit samples are stored as unsigned, but engine convention is to store all bit depths as signed
    if (mBytesPerSample == 1)
    {
        for (uint32 i = 0; i < numRead; i++)
        {
            int8 val = samples[i] - 128;
            samples[i] = *((uint8*)&val);
        }
    }
    // IEEE float need to be converted into signed PCM data
    else if (mFormat == WAVE_FORMAT_IEEE_FLOAT)
    {
        AudioTool::ConvertFromFloat((const float*)samples, (int32*)samples, numSamples);
    }
}

bool WaveDecoder::IsValid(ReadStream* stream, uint32 offset)
{
    ASSERT(stream);

    stream->SetPosition(offset);

    byte header[MAIN_CHUNK_SIZE];
    stream->ReadBytes(header, sizeof(header));

    return (header[0] == 'R') && (header[1] == 'I') && (header[2] == 'F') && (header[3] == 'F')
            && (header[8] == 'W') && (header[9] == 'A') && (header[10] == 'V') && (header[11] == 'E');
}
