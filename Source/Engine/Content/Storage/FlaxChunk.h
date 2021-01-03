// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Types/DateTime.h"
#include "../Config.h"

/// <summary>
/// Custom flags for the storage chunk data.
/// </summary>
enum class FlaxChunkFlags
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// Compress chunk data using LZ4 algorithm.
    /// </summary>
    CompressedLZ4 = 1,
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
    FlaxChunkFlags Flags;

    /// <summary>
    /// The last usage time (UTC).
    /// </summary>
    DateTime LastAccessTime;

    /// <summary>
    /// The chunk data.
    /// </summary>
    BytesContainer Data;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxChunk"/> class.
    /// </summary>
    FlaxChunk()
        : Flags(FlaxChunkFlags::None)
        , LastAccessTime(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxChunk"/> class.
    /// </summary>
    /// <param name="location">The chunk location in the file.</param>
    FlaxChunk(const Location& location)
        : LocationInFile(location)
        , Flags(FlaxChunkFlags::None)
        , LastAccessTime(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxChunk" /> class.
    /// </summary>
    /// <param name="location">The chunk location in the file.</param>
    /// <param name="flags">The flags.</param>
    FlaxChunk(const Location& location, const FlaxChunkFlags flags)
        : LocationInFile(location)
        , Flags(flags)
        , LastAccessTime(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxChunk"/> class.
    /// </summary>
    /// <param name="address">The address in the file.</param>
    /// <param name="size">The size in bytes.</param>
    FlaxChunk(uint32 address, uint32 size)
        : LocationInFile(address, size)
        , Flags(FlaxChunkFlags::None)
        , LastAccessTime(0)
    {
    }

public:

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
    /// <returns>Data</returns>
    FORCE_INLINE byte* Get()
    {
        return Data.Get();
    }

    /// <summary>
    /// Gets this chunk data pointer.
    /// </summary>
    /// <returns>Data</returns>
    FORCE_INLINE const byte* Get() const
    {
        return Data.Get();
    }

    /// <summary>
    /// Gets this chunk data pointer.
    /// </summary>
    /// <returns>Data</returns>
    template<typename T>
    FORCE_INLINE T* Get() const
    {
        return (T*)Data.Get();
    }

    /// <summary>
    /// Gets this chunk data size (in bytes).
    /// </summary>
    /// <returns>Data size</returns>
    FORCE_INLINE int32 Size() const
    {
        return Data.Length();
    }

public:

    /// <summary>
    /// Determines whether this chunk is loaded.
    /// </summary>
    /// <returns>True if this instance is loaded, otherwise false.</returns>
    FORCE_INLINE bool IsLoaded() const
    {
        return Data.IsValid();
    }

    /// <summary>
    /// Determines whether this chunk is missing (no data loaded or assigned).
    /// </summary>
    /// <returns>True if this instance is missing, otherwise false.</returns>
    FORCE_INLINE bool IsMissing() const
    {
        return Data.IsInvalid();
    }

    /// <summary>
    /// Determines whether this chunk exists in a file.
    /// </summary>
    /// <returns>True if this instance is in a file, otherwise false.</returns>
    FORCE_INLINE bool ExistsInFile() const
    {
        return LocationInFile.Size > 0;
    }

    /// <summary>
    /// Registers the usage operation of chunk data.
    /// </summary>
    void RegisterUsage()
    {
        LastAccessTime = DateTime::NowUTC();
    }

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
