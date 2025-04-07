// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "AudioEncoder.h"
#include "Engine/Audio/Config.h"

#if COMPILE_WITH_AUDIO_TOOL && COMPILE_WITH_OGG_VORBIS

#include <ThirdParty/vorbis/vorbisfile.h>

/// <summary>
/// Raw PCM data encoder to Ogg Vorbis audio format.
/// </summary>
/// <seealso cref="AudioEncoder" />
class OggVorbisEncoder : public AudioEncoder
{
public:

    typedef void (*WriteCallback)(byte*, uint32, void*);

private:

    static const uint32 BUFFER_SIZE = 4096;

    WriteCallback _writeCallback;
    void* _userData;
    byte _buffer[BUFFER_SIZE];
    uint32 _bufferOffset;
    uint32 _numChannels;
    uint32 _bitDepth;
    bool _closed;

    ogg_stream_state _oggState;
    vorbis_info _vorbisInfo;
    vorbis_dsp_state _vorbisState;
    vorbis_block _vorbisBlock;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="OggVorbisEncoder"/> class.
    /// </summary>
    OggVorbisEncoder();

    /// <summary>
    /// Finalizes an instance of the <see cref="OggVorbisEncoder"/> class.
    /// </summary>
    ~OggVorbisEncoder();

public:

    /// <summary>
    /// Sets up the writer. Should be called before calling Write().
    /// </summary>
    /// <param name="writeCallback">Callback that will be triggered when the writer is ready to output some data. The callback should copy the provided data into its own buffer.</param>
    /// <param name="sampleRate">Determines how many samples per second the written data will have.</param>
    /// <param name="bitDepth">Determines the size of a single sample, in bits.</param>
    /// <param name="numChannels">Determines the number of audio channels. Channel data will be output interleaved in the output buffer.</param>
    /// <param name="quality">The output data compression quality (normalized in range [0;1]).</param>
    /// <param name="userData">The custom used data passed to the write callback.</param>
    /// <returns>True if failed to open the data, otherwise false.</returns>
    bool Open(WriteCallback writeCallback, uint32 sampleRate, uint32 bitDepth, uint32 numChannels, float quality, void* userData = nullptr);

    /// <summary>
    /// Writes a new set of samples and converts them to Ogg Vorbis.
    /// </summary>
    /// <param name="samples">The samples in PCM format. 8-bit samples should be unsigned, but higher bit depths signed. Each sample is assumed to be the bit depth that was provided to the Open() method.</param>
    /// <param name="numSamples">The number of samples to encode.</param>
    void Write(byte* samples, uint32 numSamples);

    /// <summary>
    /// Flushes the last of the data into the write buffer (triggers the write callback). This is called automatically when the writer is closed or goes out of scope.
    /// </summary>
    void Flush();

    /// <summary>
    /// Closes the encoder and flushes the last of the data into the write buffer (triggers the write callback). This is called automatically when the writer goes out of scope.
    /// </summary>
    void Close();

private:

    /// <summary>
    /// Writes Vorbis blocks into Ogg packets.
    /// </summary>
    void WriteBlocks();

public:

    // [AudioEncoder]
    bool Convert(byte* samples, AudioDataInfo& info, BytesContainer& result, float quality = 0.5f) override;
};

#endif
