// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_AUDIO_TOOL

#include "AudioDecoder.h"
#include "Engine/Serialization/ReadStream.h"
#include <minimp3/minimp3.h>

/// <summary>
/// Decodes .mp3 audio data into raw PCM format.
/// </summary>
/// <seealso cref="AudioDecoder" />
class MP3Decoder : public AudioDecoder
{
private:

    ReadStream* mStream;
    mp3dec_t mp3d;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MP3Decoder"/> class.
    /// </summary>
    MP3Decoder()
    {
        mStream = nullptr;
        mp3dec_init(&mp3d);
    }

public:

    // [AudioDecoder]
    bool Convert(ReadStream* stream, AudioDataInfo& info, Array<byte>& result, uint32 offset = 0) override;
    bool Open(ReadStream* stream, AudioDataInfo& info, uint32 offset = 0) override;
    void Seek(uint32 offset) override;
    void Read(byte* samples, uint32 numSamples) override;
    bool IsValid(ReadStream* stream, uint32 offset = 0) override;
};

#endif
