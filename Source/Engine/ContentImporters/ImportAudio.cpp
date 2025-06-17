// Copyright (c) Wojciech Figat. All rights reserved.

#include "ImportAudio.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/DeleteMe.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Tools/AudioTool/AudioTool.h"
#include "Engine/Tools/AudioTool/MP3Decoder.h"
#include "Engine/Tools/AudioTool/WaveDecoder.h"
#include "Engine/Tools/AudioTool/OggVorbisDecoder.h"
#include "Engine/Tools/AudioTool/OggVorbisEncoder.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Platform/MessageBox.h"

bool ImportAudio::TryGetImportOptions(const StringView& path, Options& options)
{
#if IMPORT_AUDIO_CACHE_OPTIONS
    if (FileSystem::FileExists(path))
    {
        auto tmpFile = ContentStorageManager::GetStorage(path);
        AssetInitData data;
        if (tmpFile
            && tmpFile->GetEntriesCount() == 1
            && tmpFile->GetEntry(0).TypeName == AudioClip::TypeName
            && !tmpFile->LoadAssetHeader(0, data)
            && data.SerializedVersion >= 1)
        {
            // Check import meta
            rapidjson_flax::Document metadata;
            metadata.Parse(data.Metadata.Get<const char>(), data.Metadata.Length());
            if (!metadata.HasParseError())
            {
                options.Deserialize(metadata, nullptr);
                return true;
            }
        }
    }
#endif
    return false;
}

CreateAssetResult ImportAudio::Import(CreateAssetContext& context, AudioDecoder& decoder)
{
    // Get import options
    Options options;
    if (context.CustomArg != nullptr)
    {
        // Copy import options from argument
        options = *static_cast<Options*>(context.CustomArg);
    }
    else
    {
        // Restore the previous settings or use default ones
        if (!TryGetImportOptions(context.TargetAssetPath, options))
        {
            LOG(Warning, "Missing audio import options. Using default values.");
        }
    }

    // Vorbis uses fixed 16-bit depth
    if (options.Format == AudioFormat::Vorbis)
        options.BitDepth = AudioTool::BitDepth::_16;

    LOG_STR(Info, options.ToString());

    // Open the file
    auto stream = FileReadStream::Open(context.InputPath);
    if (stream == nullptr)
        return CreateAssetResult::InvalidPath;
    DeleteMe<FileReadStream> deleteStream(stream);

    // Load the audio data
    AudioDataInfo info;
    Array<byte> audioData;
    if (decoder.Convert(stream, info, audioData))
        return CreateAssetResult::Error;
    LOG(Info, "Audio: {0}kHz, channels: {1}, Bit depth: {2}, Length: {3}s", info.SampleRate / 1000.0f, info.NumChannels, info.BitDepth, info.GetLength());

    // Load the whole audio data
    uint32 bytesPerSample = info.BitDepth / 8;
    uint32 bufferSize = info.NumSamples * bytesPerSample;
    DataContainer<byte> sampleBuffer;
    sampleBuffer.Link(audioData.Get());

    // Convert bit depth if need to
    uint32 outputBitDepth = (uint32)options.BitDepth;
    if (outputBitDepth != info.BitDepth)
    {
        const uint32 outBufferSize = info.NumSamples * (outputBitDepth / 8);
        sampleBuffer.Allocate(outBufferSize);
        AudioTool::ConvertBitDepth(audioData.Get(), info.BitDepth, sampleBuffer.Get(), outputBitDepth, info.NumSamples);
        info.BitDepth = outputBitDepth;
        bytesPerSample = info.BitDepth / 8;
        bufferSize = outBufferSize;
    }

    // Base
    IMPORT_SETUP(AudioClip, AudioClip::SerializedVersion);
    uint32 samplesPerChunk[ASSET_FILE_DATA_CHUNKS];
    Platform::MemoryClear(samplesPerChunk, sizeof(samplesPerChunk));

    // Helper macro to write audio data to chunk (handles per chunk compression)
#if COMPILE_WITH_OGG_VORBIS
#define HANDLE_VORBIS(chunkIndex, dataPtr, dataSize) \
    infoEx.NumSamples = samplesPerChunk[chunkIndex]; \
    OggVorbisEncoder encoder; \
    if (encoder.Convert(dataPtr, infoEx, context.Data.Header.Chunks[chunkIndex]->Data, options.Quality)) \
    { \
	    LOG(Warning, "Failed to compress audio data"); \
	    return CreateAssetResult::Error; \
    }
#else
#define HANDLE_VORBIS(chunkIndex, dataPtr, dataSize) \
    MessageBox::Show(TEXT("Vorbis format is not supported."), TEXT("Import warning"), MessageBoxButtons::OK, MessageBoxIcon::Warning);
	LOG(Warning, "Vorbis format is not supported."); \
	return CreateAssetResult::Error;
#endif
#define HANDLE_RAW(chunkIndex, dataPtr, dataSize) \
    context.Data.Header.Chunks[chunkIndex]->Data.Copy(dataPtr, dataSize);

#define WRITE_DATA(chunkIndex, dataPtr, dataSize) \
    samplesPerChunk[chunkIndex] = (dataSize) / (outputBitDepth / 8); \
    switch (options.Format) \
    { \
    case AudioFormat::Raw: \
    { \
        HANDLE_RAW(chunkIndex, dataPtr, dataSize); \
    } \
    break; \
    case AudioFormat::Vorbis: \
    { \
        HANDLE_VORBIS(chunkIndex, dataPtr, dataSize); \
    } \
    break; \
    default: \
    { \
        MessageBox::Show(TEXT("Unknown audio format."), TEXT("Import warning"), MessageBoxButtons::OK, MessageBoxIcon::Warning); \
        LOG(Warning, "Unknown audio format."); \
        return CreateAssetResult::Error; \
    } \
    break; \
    }
    AudioDataInfo infoEx = info;

    // If audio has streaming disabled then use a single chunk
    if (options.DisableStreaming)
    {
        // Copy data
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;

        WRITE_DATA(0, sampleBuffer.Get(), bufferSize);
    }
    else
    {
        // Split audio data into a several chunks (uniform data spread)
        const uint32 minChunkSize = 1 * 1024 * 1024; // 1 MB
        const uint32 dataAlignment = info.NumChannels * bytesPerSample * ASSET_FILE_DATA_CHUNKS; // Ensure to never split samples in-between (eg. 24-bit that uses 3 bytes)
        const uint32 chunkSize = Math::AlignUp(Math::Max(minChunkSize, bufferSize / ASSET_FILE_DATA_CHUNKS), dataAlignment);
        const int32 chunksCount = Math::CeilToInt((float)bufferSize / (float)chunkSize);
        ASSERT(chunksCount > 0 && chunksCount <= ASSET_FILE_DATA_CHUNKS);

        byte* ptr = sampleBuffer.Get();
        uint32 size = bufferSize;
        for (int32 chunkIndex = 0; chunkIndex < chunksCount; chunkIndex++)
        {
            if (context.AllocateChunk(chunkIndex))
                return CreateAssetResult::CannotAllocateChunk;
            const uint32 t = Math::Min(size, chunkSize);

            WRITE_DATA(chunkIndex, ptr, t);

            ptr += t;
            size -= t;
        }
        ASSERT(size == 0);
    }

    // Save audio header
    {
        AudioClip::Header header;
        header.Format = options.Format;
        header.Info = info;
        header.Is3D = options.Is3D;
        header.Streamable = !options.DisableStreaming;
        header.OriginalSize = stream->GetLength();
        Platform::MemoryCopy(header.SamplesPerChunk, samplesPerChunk, sizeof(samplesPerChunk));
        static_assert(AudioClip::SerializedVersion == 2, "Update this code to match the audio clip header format.");
        header.ImportedSize = 0;
        for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        {
            if (context.Data.Header.Chunks[i])
                header.ImportedSize += context.Data.Header.Chunks[i]->Size();
        }
        context.Data.CustomData.Copy(&header);
    }

    // Create json with import context
    rapidjson_flax::StringBuffer importOptionsMetaBuffer;
    importOptionsMetaBuffer.Reserve(256);
    CompactJsonWriter importOptionsMetaObj(importOptionsMetaBuffer);
    JsonWriter& importOptionsMeta = importOptionsMetaObj;
    importOptionsMeta.StartObject();
    {
        context.AddMeta(importOptionsMeta);
        options.Serialize(importOptionsMeta, nullptr);
    }
    importOptionsMeta.EndObject();
    context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportAudio::ImportWav(CreateAssetContext& context)
{
    WaveDecoder decoder;
    return Import(context, decoder);
}

CreateAssetResult ImportAudio::ImportMp3(CreateAssetContext& context)
{
    MP3Decoder decoder;
    return Import(context, decoder);
}

#if COMPILE_WITH_OGG_VORBIS

CreateAssetResult ImportAudio::ImportOgg(CreateAssetContext& context)
{
    OggVorbisDecoder decoder;
    return Import(context, decoder);
}

#endif

#endif
