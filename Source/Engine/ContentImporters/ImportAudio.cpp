// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

ImportAudio::Options::Options()
{
    Format = AudioFormat::Vorbis;
    DisableStreaming = false;
    Is3D = false;
    Quality = 0.4f;
    BitDepth = 16;
}

String ImportAudio::Options::ToString() const
{
    return String::Format(TEXT("Format:{}, DisableStreaming:{}, Is3D:{}, Quality:{}, BitDepth:{}"),
                          ::ToString(Format),
                          DisableStreaming,
                          Is3D,
                          Quality,
                          BitDepth
    );
}

void ImportAudio::Options::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(ImportAudio::Options);

    SERIALIZE(Format);
    SERIALIZE(DisableStreaming);
    SERIALIZE(Is3D);
    SERIALIZE(Quality);
    SERIALIZE(BitDepth);
}

void ImportAudio::Options::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Format);
    DESERIALIZE(DisableStreaming);
    DESERIALIZE(Is3D);
    DESERIALIZE(Quality);
    DESERIALIZE(BitDepth);
}

bool ImportAudio::TryGetImportOptions(String path, Options& options)
{
#if IMPORT_AUDIO_CACHE_OPTIONS

    // Check if target asset file exists
    if (FileSystem::FileExists(path))
    {
        // Try to load asset file and asset info
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
                // Success
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

    const float length = info.NumSamples / static_cast<float>(Math::Max(1U, info.SampleRate * info.NumChannels));
    LOG(Info, "Audio: {0}kHz, channels: {1}, Bit depth: {2}, Length: {3}s", info.SampleRate / 1000.0f, info.NumChannels, info.BitDepth, length);

    // Load the whole audio data
    uint32 bytesPerSample = info.BitDepth / 8;
    uint32 bufferSize = info.NumSamples * bytesPerSample;
    DataContainer<byte> sampleBuffer;
    sampleBuffer.Link(audioData.Get());

    // Convert to Mono if used as 3D source
    if (options.Is3D && info.NumChannels > 1)
    {
        const uint32 numSamplesPerChannel = info.NumSamples / info.NumChannels;

        const uint32 monoBufferSize = numSamplesPerChannel * bytesPerSample;
        sampleBuffer.Allocate(monoBufferSize);

        AudioTool::ConvertToMono(audioData.Get(), sampleBuffer.Get(), info.BitDepth, numSamplesPerChannel, info.NumChannels);

        info.NumSamples = numSamplesPerChannel;
        info.NumChannels = 1;

        bufferSize = monoBufferSize;
    }

    // Convert bit depth if need to
    if (options.BitDepth != static_cast<int32>(info.BitDepth))
    {
        const uint32 outBufferSize = info.NumSamples * (options.BitDepth / 8);
        sampleBuffer.Allocate(outBufferSize);

        AudioTool::ConvertBitDepth(audioData.Get(), info.BitDepth, sampleBuffer.Get(), options.BitDepth, info.NumSamples);

        info.BitDepth = options.BitDepth;
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
	LOG(Warning, "Vorbis format is not supported."); \
	return CreateAssetResult::Error;
#endif
#define HANDLE_RAW(chunkIndex, dataPtr, dataSize) \
    context.Data.Header.Chunks[chunkIndex]->Data.Copy(dataPtr, dataSize);

#define WRITE_DATA(chunkIndex, dataPtr, dataSize) \
    samplesPerChunk[chunkIndex] = (dataSize) / (options.BitDepth / 8); \
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
        const int32 MinChunkSize = 1 * 1024 * 1024; // 1 MB
        const int32 chunkSize = Math::Max<int32>(MinChunkSize, Math::AlignUp<uint32>(bufferSize / ASSET_FILE_DATA_CHUNKS, 256));
        const int32 chunksCount = Math::CeilToInt((float)bufferSize / chunkSize);
        ASSERT(chunksCount > 0 && chunksCount <= ASSET_FILE_DATA_CHUNKS);

        byte* ptr = sampleBuffer.Get();
        int32 size = bufferSize;
        for (int32 chunkIndex = 0; chunkIndex < chunksCount; chunkIndex++)
        {
            if (context.AllocateChunk(chunkIndex))
                return CreateAssetResult::CannotAllocateChunk;
            const int32 t = Math::Min(size, chunkSize);

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
