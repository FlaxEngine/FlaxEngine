// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_AUDIO_TOOL

#include "AudioDecoder.h"
#include "Engine/Serialization/ReadStream.h"

/// <summary>
/// Decodes .wav audio data into raw PCM format.
/// </summary>
/// <seealso cref="AudioDecoder" />
class WaveDecoder : public AudioDecoder
{
private:

    ReadStream* mStream;
    uint16 mFormat;
    uint32 mDataOffset;
    uint32 mBytesPerSample;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WaveDecoder"/> class.
    /// </summary>
    WaveDecoder()
    {
        mStream = nullptr;
        mFormat = 0;
        mDataOffset = 0;
        mBytesPerSample = 0;
    }

private:

    /// <summary>
    /// Parses the WAVE header and output audio file meta-data. Returns false if the header is not valid.
    /// </summary>
    /// <param name="info">The output information.</param>
    /// <returns>True if header is valid, otherwise false.</returns>
    bool ParseHeader(AudioDataInfo& info);

public:

    // [AudioDecoder]
    bool Open(ReadStream* stream, AudioDataInfo& info, uint32 offset = 0) override;
    void Seek(uint32 offset) override;
    void Read(byte* samples, uint32 numSamples) override;
    bool IsValid(ReadStream* stream, uint32 offset = 0) override;
};

#endif
