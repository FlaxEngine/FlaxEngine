// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Audio/Types.h"
#include "Engine/Core/Collections/Array.h"

class ReadStream;

/// <summary>
/// Interface used for implementations that parse audio formats into a set of PCM samples.
/// </summary>
class AudioDecoder
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="AudioDecoder"/> class.
    /// </summary>
    virtual ~AudioDecoder()
    {
    }

public:

    /// <summary>
    /// Tries to open the specified stream with audio data and loads the whole audio data.
    /// </summary>
    /// <param name="stream">The data stream audio data is stored in. Must be valid until decoder usage end. Decoder may cache this pointer for the later usage.</param>
    /// <param name="info">The output information describing meta-data of the audio in the stream.</param>
    /// <param name="result">The output data.</param>
    /// <param name="offset">The offset.</param>
    /// <returns>True if the data is invalid or conversion failed, otherwise false.</returns>
    virtual bool Convert(ReadStream* stream, AudioDataInfo& info, Array<byte>& result, uint32 offset = 0)
    {
        if (!IsValid(stream, offset))
            return true;
        if (!Open(stream, info, offset))
            return true;

        // Load the whole audio data
        const int32 bytesPerSample = info.BitDepth / 8;
        const int32 bufferSize = info.NumSamples * bytesPerSample;
        result.Resize(bufferSize);
        Read(result.Get(), info.NumSamples);

        return false;
    }

public:

    /// <summary>
    /// Tries to open the specified stream with audio data. Must be called before any reads or seeks.
    /// </summary>
    /// <param name="stream">The data stream audio data is stored in. Must be valid until decoder usage end. Decoder may cache this pointer for the later usage.</param>
    /// <param name="info">The output information describing meta-data of the audio in the stream.</param>
    /// <param name="offset">The offset.</param>
    /// <returns>True if the data is invalid, otherwise false.</returns>
    virtual bool Open(ReadStream* stream, AudioDataInfo& info, uint32 offset = 0) = 0;

    /// <summary>
    /// Moves the read pointer to the specified offset. Any further Read() calls will read from this location. User must ensure not to seek past the end of the data.
    /// </summary>
    /// <param name="offset">The offset to move the pointer in. In number of samples.</param>
    virtual void Seek(uint32 offset) = 0;

    /// <summary>
    /// Reads a set of samples from the audio data.
    /// </summary>
    /// <remarks>
    /// All values are returned as signed values.
    /// </remarks>
    /// <param name="samples">Pre-allocated buffer to store the samples in.</param>
    /// <param name="numSamples">The number of samples to read.</param>
    virtual void Read(byte* samples, uint32 numSamples) = 0;

    /// <summary>
    /// Checks if the data in the provided stream valid audio data for the current format. You should check this before calling Open().
    /// </summary>
    /// <param name="stream">The stream to check.</param>
    /// <param name="offset">The offset at which audio data in the stream begins, in bytes.</param>
    /// <returns>True if the data is valid, otherwise false.</returns>
    virtual bool IsValid(ReadStream* stream, uint32 offset = 0) = 0;
};
