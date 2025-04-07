// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "../Config.h"

/// <summary>
/// Custom flags for the storage chunk data.
/// </summary>
enum class FlaxChunkFlags
{
    /// <summary>
    /// Nothing.
    /// </summary>
    None = 0,

    /// <summary>
    /// Compress chunk data using LZ4 algorithm.
    /// </summary>
    CompressedLZ4 = 1,

    /// <summary>
    /// Prevents chunk file data from being unloaded if unused for a certain amount of time. Runtime-only flag, not saved with the asset.
    /// </summary>
    KeepInMemory = 2,
};

DECLARE_ENUM_OPERATORS(FlaxChunkFlags);

/// <summary>
/// Represents chunks of data used by the content storage layer
/// </summary>
class FLAXENGINE_API FlaxChunk
{
public:
    /// <summary>
    /// Chunk of data location info
    /// </summary>
    struct Location
    {
        /// <summary>
        /// Address of the chunk beginning in file
        /// </summary>
        uint32 Address;

        /// <summary>
        /// Chunk size (in bytes)
        /// Note: chunk which size equals 0 is considered as not existing
        /// </summary>
        uint32 Size;

        /// <summary>
        /// Init
        /// </summary>
        Location()
            : Address(0)
            , Size(0)
        {
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="size">The size.</param>
        Location(uint32 location, uint32 size)
            : Address(location)
            , Size(size)
        {
        }
    };

public:
    /// <summary>
    /// The chunk location in file.
    /// </summary>
    Location LocationInFile;

    /// <summary>
    /// The chunk flags.
    /// </summary>
    FlaxChunkFlags Flags = FlaxChunkFlags::None;

    /// <summary>
    /// The last usage time.
    /// </summary>
    double LastAccessTime = 0.0;

    /// <summary>
    /// The chunk data.
    /// </summary>
    BytesContainer Data;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxChunk"/> class.
    /// </summary>
    FlaxChunk()
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="FlaxChunk"/> class.
    /// </summary>
    ~FlaxChunk()
    {
    }

public:
    /// <summary>
    /// Gets this chunk data pointer.
    /// </summary>
    FORCE_INLINE byte* Get()
    {
        return Data.Get();
    }

    /// <summary>
    /// Gets this chunk data pointer.
    /// </summary>
    FORCE_INLINE const byte* Get() const
    {
        return Data.Get();
    }

    /// <summary>
    /// Gets this chunk data pointer.
    /// </summary>
    template<typename T>
    FORCE_INLINE T* Get() const
    {
        return (T*)Data.Get();
    }

    /// <summary>
    /// Gets this chunk data size (in bytes).
    /// </summary>
    FORCE_INLINE int32 Size() const
    {
        return Data.Length();
    }

    /// <summary>
    /// Determines whether this chunk is loaded.
    /// </summary>
    FORCE_INLINE bool IsLoaded() const
    {
        return Data.IsValid();
    }

    /// <summary>
    /// Determines whether this chunk is missing (no data loaded or assigned).
    /// </summary>
    FORCE_INLINE bool IsMissing() const
    {
        return Data.IsInvalid();
    }

    /// <summary>
    /// Determines whether this chunk exists in a file.
    /// </summary>
    FORCE_INLINE bool ExistsInFile() const
    {
        return LocationInFile.Size > 0;
    }

    /// <summary>
    /// Registers the usage operation of chunk data.
    /// </summary>
    void RegisterUsage();

    /// <summary>
    /// Unloads this chunk data.
    /// </summary>
    void Unload()
    {
        Data.Release();
    }

    /// <summary>
    /// Clones this chunk data (doesn't copy location in file).
    /// </summary>
    /// <returns>The cloned chunk.</returns>
    FlaxChunk* Clone() const
    {
        auto chunk = New<FlaxChunk>();
        chunk->Data.Copy(Data);
        return chunk;
    }
};
