// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Serialization/Stream.h"

/// <summary>
/// Visject metadata container
/// </summary>
class FLAXENGINE_API VisjectMeta
{
public:
    /// <summary>
    /// Metadata entry
    /// </summary>
    struct Entry
    {
        int32 TypeID;
        bool IsLoaded;
        Array<byte> Data;
    };

public:
    /// <summary>
    /// All meta entries
    /// </summary>
    Array<Entry, FixedAllocation<8>> Entries;

public:
    /// <summary>
    /// Load from the stream
    /// </summary>
    /// <param name="stream">Stream</param>
    /// <param name="loadData">True if load meta data</param>
    /// <returns>True if cannot load data</returns>
    bool Load(ReadStream* stream, bool loadData);

    /// <summary>
    /// Save to the stream
    /// </summary>
    /// <param name="stream">Stream</param>
    /// <param name="saveData">True if load meta data</param>
    /// <returns>True if cannot save data</returns>
    bool Save(WriteStream* stream, bool saveData) const;

    /// <summary>
    /// Release meta data
    /// </summary>
    void Release();

    /// <summary>
    /// Get entry
    /// </summary>
    /// <param name="typeID">Entry type ID</param>
    /// <returns>Entry</returns>
    const Entry* GetEntry(int32 typeID) const;

    /// <summary>
    /// Get entry
    /// </summary>
    /// <param name="typeID">Entry type ID</param>
    /// <returns>Entry</returns>
    Entry* GetEntry(int32 typeID);

    /// <summary>
    /// Add new entry
    /// </summary>
    /// <param name="typeID">Type ID</param>
    /// <param name="data">Bytes to set</param>
    /// <param name="size">Amount of bytes to assign</param>
    void AddEntry(int32 typeID, byte* data, int32 size);
};
