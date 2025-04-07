// Copyright (c) Wojciech Figat. All rights reserved.

#include "AssetExporters.h"

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Engine/Core/Log.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Platform/File.h"
#include "Engine/Tools/AudioTool/OggVorbisEncoder.h"

ExportAssetResult AssetExporters::ExportAudioClip(ExportAssetContext& context)
{
#if COMPILE_WITH_OGG_VORBIS
    // Prepare
    auto asset = (AudioClip*)context.Asset.Get();
    auto lock = asset->Storage->LockSafe();
    auto path = GET_OUTPUT_PATH(context, "ogg");

    // Get audio data
    Array<byte> rawData;
    AudioDataInfo rawDataInfo;
    if (asset->ExtractDataRaw(rawData, rawDataInfo))
        return ExportAssetResult::CannotLoadData;

    // Encode PCM data
    BytesContainer encodedData;
    OggVorbisEncoder encoder;
    if (encoder.Convert(rawData.Get(), rawDataInfo, encodedData, 1.0f))
    {
        LOG(Warning, "Failed to compress audio data");
        return ExportAssetResult::Error;
    }

    // Save to file
    if (File::WriteAllBytes(path, encodedData.Get(), encodedData.Length()))
    {
        LOG(Warning, "Failed to save data to file");
        return ExportAssetResult::Error;
    }

    return ExportAssetResult::Ok;
#else
	LOG(Warning, "OggVorbis support is disabled.");
	return ExportAssetResult::Error;
#endif
}

#endif
