// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_OGG_VORBIS

#include "AudioDecoder.h"
#include "Engine/Serialization/ReadStream.h"
#include <ThirdParty/vorbis/vorbisfile.h>

/// <summary>
/// Decodes .ogg audio data into raw PCM format.
/// </summary>
/// <seealso cref="AudioDecoder" />
class OggVorbisDecoder : public AudioDecoder
{
public:

    ReadStream* Stream;
    uint32 Offset;
    uint32 ChannelCount;
    OggVorbis_File OggVorbisFile;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="OggVorbisDecoder"/> class.
    /// </summary>
    OggVorbisDecoder()
    {
        Stream = nullptr;
        Offset = 0;
        ChannelCount = 0;
        OggVorbisFile.datasource = nullptr;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="OggVorbisDecoder"/> class.
    /// </summary>
    ~OggVorbisDecoder()
    {
        if (OggVorbisFile.datasource != nullptr)
            ov_clear(&OggVorbisFile);
    }

public:

    // [AudioDecoder]
    bool Open(ReadStream* stream, AudioDataInfo& info, uint32 offset = 0) override;
    void Seek(uint32 offset) override;
    void Read(byte* samples, uint32 numSamples) override;
    bool IsValid(ReadStream* stream, uint32 offset = 0) override;
};

#endif
