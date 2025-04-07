// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Tools/AudioTool/AudioTool.h"
#include "Engine/Tools/AudioTool/AudioDecoder.h"

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
    typedef AudioTool::Options Options;

public:
    /// <summary>
    /// Tries the get audio import options from the target location asset.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="options">The options.</param>
    /// <returns>True if success, otherwise false.</returns>
    static bool TryGetImportOptions(const StringView& path, Options& options);

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
