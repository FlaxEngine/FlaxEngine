// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Tools/AudioTool/AudioDecoder.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Audio/Config.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Enable/disable caching audio import options
/// </summary>
#define IMPORT_AUDIO_CACHE_OPTIONS 1

/// <summary>
/// Importing audio utility
/// </summary>
class ImportAudio
{
public:
    /// <summary>
    /// Importing audio options
    /// </summary>
    struct Options : public ISerializable
    {
        AudioFormat Format = AudioFormat::Vorbis;
        bool DisableStreaming = false;
        bool Is3D = false;
        int32 BitDepth = 16;
        float Quality = 0.4f;

        String ToString() const;

        // [ISerializable]
        void Serialize(SerializeStream& stream, const void* otherObj) override;
        void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    };

public:
    /// <summary>
    /// Tries the get audio import options from the target location asset.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="options">The options.</param>
    /// <returns>True if success, otherwise false.</returns>
    static bool TryGetImportOptions(const String& path, Options& options);

    /// <summary>
    /// Imports the audio data (with given audio decoder).
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <param name="decoder">The audio decoder.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Import(CreateAssetContext& context, AudioDecoder& decoder);

    /// <summary>
    /// Imports the .wav audio file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportWav(CreateAssetContext& context);

    /// <summary>
    /// Imports the .mp3 audio file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportMp3(CreateAssetContext& context);

#if COMPILE_WITH_OGG_VORBIS
    /// <summary>
    /// Imports the .ogg audio file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportOgg(CreateAssetContext& context);
#endif
};

#endif
