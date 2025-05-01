// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_OGG_VORBIS

#include "OggVorbisEncoder.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "AudioTool.h"
#include <ThirdParty/vorbis/vorbisenc.h>

// Writes to the internal cached buffer and flushes it if needed
#define WRITE_TO_BUFFER(data, length) \
	if ((_bufferOffset + length) > BUFFER_SIZE) \
		Flush(); \
	if(length > BUFFER_SIZE) \
		_writeCallback(data, length, _userData); \
	else \
	{ \
		Platform::MemoryCopy(_buffer + _bufferOffset, data, length); \
		_bufferOffset += length; \
	}

OggVorbisEncoder::OggVorbisEncoder()
    : _bufferOffset(0)
    , _numChannels(0)
    , _bitDepth(0)
    , _closed(true)
{
}

OggVorbisEncoder::~OggVorbisEncoder()
{
    Close();
}

bool OggVorbisEncoder::Open(WriteCallback writeCallback, uint32 sampleRate, uint32 bitDepth, uint32 numChannels, float quality, void* userData)
{
    _numChannels = numChannels;
    _bitDepth = bitDepth;
    _writeCallback = writeCallback;
    _userData = userData;
    _closed = false;

    ogg_stream_init(&_oggState, rand());
    vorbis_info_init(&_vorbisInfo);

    int32 status = vorbis_encode_init_vbr(&_vorbisInfo, numChannels, sampleRate, quality);
    if (status != 0)
    {
        LOG(Warning, "Failed to write Ogg Vorbis file.");
        Close();
        return true;
    }

    vorbis_analysis_init(&_vorbisState, &_vorbisInfo);
    vorbis_block_init(&_vorbisState, &_vorbisBlock);

    // Generate header
    vorbis_comment comment;
    vorbis_comment_init(&comment);

    ogg_packet headerPacket, commentPacket, codePacket;
    status = vorbis_analysis_headerout(&_vorbisState, &comment, &headerPacket, &commentPacket, &codePacket);
    vorbis_comment_clear(&comment);

    if (status != 0)
    {
        LOG(Warning, "Failed to write Ogg Vorbis file.");
        Close();
        return true;
    }

    // Write header
    ogg_stream_packetin(&_oggState, &headerPacket);
    ogg_stream_packetin(&_oggState, &commentPacket);
    ogg_stream_packetin(&_oggState, &codePacket);

    ogg_page page;
    while (ogg_stream_flush(&_oggState, &page) > 0)
    {
        WRITE_TO_BUFFER(page.header, page.header_len);
        WRITE_TO_BUFFER(page.body, page.body_len);
    }

    return false;
}

void OggVorbisEncoder::Write(uint8* samples, uint32 numSamples)
{
    static const uint32 WRITE_LENGTH = 1024;

    uint32 numFrames = numSamples / _numChannels;
    while (numFrames > 0)
    {
        const uint32 numFramesToWrite = Math::Min(numFrames, WRITE_LENGTH);
        float** buffer = vorbis_analysis_buffer(&_vorbisState, numFramesToWrite);

        if (_bitDepth == 8)
        {
            for (uint32 i = 0; i < numFramesToWrite; i++)
            {
                for (uint32 j = 0; j < _numChannels; j++)
                {
                    const int8 sample = *(int8*)samples;
                    const float encodedSample = sample / 127.0f;
                    buffer[j][i] = encodedSample;

                    samples++;
                }
            }
        }
        else if (_bitDepth == 16)
        {
            for (uint32 i = 0; i < numFramesToWrite; i++)
            {
                for (uint32 j = 0; j < _numChannels; j++)
                {
                    const int16 sample = *(int16*)samples;
                    const float encodedSample = sample / 32767.0f;
                    buffer[j][i] = encodedSample;

                    samples += 2;
                }
            }
        }
        else if (_bitDepth == 24)
        {
            for (uint32 i = 0; i < numFramesToWrite; i++)
            {
                for (uint32 j = 0; j < _numChannels; j++)
                {
                    const int32 sample = AudioTool::Convert24To32Bits(samples);
                    const float encodedSample = sample / 2147483647.0f;
                    buffer[j][i] = encodedSample;

                    samples += 3;
                }
            }
        }
        else if (_bitDepth == 32)
        {
            for (uint32 i = 0; i < numFramesToWrite; i++)
            {
                for (uint32 j = 0; j < _numChannels; j++)
                {
                    const int32 sample = *(int32*)samples;
                    const float encodedSample = sample / 2147483647.0f;
                    buffer[j][i] = encodedSample;

                    samples += 4;
                }
            }
        }
        else
        {
            CRASH;
        }

        // Signal how many frames were written
        vorbis_analysis_wrote(&_vorbisState, numFramesToWrite);
        WriteBlocks();

        numFrames -= numFramesToWrite;
    }
}

void OggVorbisEncoder::WriteBlocks()
{
    while (vorbis_analysis_blockout(&_vorbisState, &_vorbisBlock) == 1)
    {
        // Analyze and determine optimal bit rate
        vorbis_analysis(&_vorbisBlock, nullptr);
        vorbis_bitrate_addblock(&_vorbisBlock);

        // Write block into ogg packets
        ogg_packet packet;
        while (vorbis_bitrate_flushpacket(&_vorbisState, &packet))
        {
            ogg_stream_packetin(&_oggState, &packet);

            // If new page, write it to the internal buffer
            ogg_page page;
            while (ogg_stream_flush(&_oggState, &page) > 0)
            {
                WRITE_TO_BUFFER(page.header, page.header_len);
                WRITE_TO_BUFFER(page.body, page.body_len);
            }
        }
    }
}

struct EncodedBlock
{
    uint8* data;
    uint32 size;
};

struct ConvertWriteCallbackData
{
    Array<EncodedBlock> Blocks;
    uint32 TotalEncodedSize = 0;
};

void ConvertWriteCallback(uint8* buffer, uint32 size, void* userData)
{
    EncodedBlock newBlock;
    newBlock.data = (uint8*)Allocator::Allocate(size);
    newBlock.size = size;

    Platform::MemoryCopy(newBlock.data, buffer, size);

    auto data = (ConvertWriteCallbackData*)userData;
    data->Blocks.Add(newBlock);
    data->TotalEncodedSize += size;
};

bool OggVorbisEncoder::Convert(byte* samples, AudioDataInfo& info, BytesContainer& result, float quality)
{
    ConvertWriteCallbackData data;

    if (Open(ConvertWriteCallback, info.SampleRate, info.BitDepth, info.NumChannels, quality, &data))
        return true;
    Write(samples, info.NumSamples);
    Close();

    result.Allocate(data.TotalEncodedSize);
    uint32 offset = 0;
    for (auto& block : data.Blocks)
    {
        Platform::MemoryCopy(result.Get() + offset, block.data, block.size);
        offset += block.size;

        Allocator::Free(block.data);
    }

    return false;
}

void OggVorbisEncoder::Flush()
{
    if (_bufferOffset > 0 && _writeCallback != nullptr)
        _writeCallback(_buffer, _bufferOffset, _userData);

    _bufferOffset = 0;
}

void OggVorbisEncoder::Close()
{
    if (_closed)
        return;

    // Mark end of data and flush any remaining data in the buffers
    vorbis_analysis_wrote(&_vorbisState, 0);
    WriteBlocks();
    Flush();

    ogg_stream_clear(&_oggState);
    vorbis_block_clear(&_vorbisBlock);
    vorbis_dsp_clear(&_vorbisState);
    vorbis_info_clear(&_vorbisInfo);

    _closed = true;
}

#endif
